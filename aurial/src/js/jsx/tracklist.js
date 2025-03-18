import { h, Component } from 'preact';
import {IconMessage,CoverArt} from './common'
import {SecondsToTime} from '../util'

export default class TrackList extends Component {

	constructor(props, context) {
		super(props, context);
		this.state = {
			queue: props.tracks,
			currentsongid: -1
		}
		props.events.subscribe({
			subscriber: this,
			event: ["mpdstatus"]
		});
	}
	//当queue组件更新queue时，本地组件也同时更新queue
    componentDidUpdate(prevProps) {
        if (prevProps.tracks!== this.props.tracks) {
			this.setState({queue:this.props.tracks});
		}
	}	

	receive(event) {
		switch (event.event) {
			case "mpdstatus":
				if(event.data.currentsongid != this.state.currentsongid){
					var track =null;
					for (const find_track of this.state.queue) {
						if (find_track.queue_sid === event.data.currentsongid) {
							track = find_track;
							break;
						}
					}
					if(track != null){//需在queue里找到新的sid才做变更
						this.props.events.publish({event: "songchange",data:track});
						this.setState({currentsongid:event.data.currentsongid});
					}
				}
			break;
		}
	}

	render() {
		var tracks = []
		if (this.props.tracks && this.props.tracks.length > 0) {
			tracks = this.props.tracks.map(function (entry) {
				return (
					<Track key={entry.id} subsonic={this.props.subsonic} events={this.props.events} track={entry}
						playing={this.state.currentsongid == entry.queue_sid}
						queued={'queue_sid' in entry} playlist={this.props.playlist}
						iconSize={this.props.iconSize} />
				);
			}.bind(this));
		}

		return (
			<table className="ui selectable single line very basic compact table trackList">
				<thead>
					<tr>
						<th className="controls">&nbsp;</th>
						<th className="number">#</th>
						<th className="artist">Artist</th>
						<th className="title">Title</th>
						<th className="album">Album</th>
						<th className="date">Date</th>
						<th className="right aligned duration">Duration</th>
					</tr>
				</thead>
				<tbody>
					{tracks}
				</tbody>
			</table>
		);
	}
}

class Track extends Component {
	constructor(props, context) {
		super(props, context);

		this.play = this.play.bind(this);
		this.enqueue = this.enqueue.bind(this);
		this.playlistAdd = this.playlistAdd.bind(this);
		this.playlistRemove = this.playlistRemove.bind(this);
	}

	play() {
		if('queue_sid' in this.props.track){//在queue中播放当前歌曲
			this.props.events.publish({event: "playtrack", data: {queue_sid: this.props.track.queue_sid}});
		}else{//在playlist中加入queue并且播放加入的歌曲
			this.props.events.publish({event: "playerEnqueue", data: {action: "ADDPLAY", tracks: [this.props.track]}});
		}
	}

	enqueue() {
		if('queue_sid' in this.props.track){//在queue中删除该歌曲
			this.props.events.publish({event: "playerEnqueue", data: {action: "DEL", tracks: this.props.track}});
		}else{//在playlist中加入queue
			this.props.events.publish({event: "playerEnqueue", data: {action: "ADD", tracks: [this.props.track]}});
		}		
	}

	playlistAdd() {
		this.props.events.publish({event: "playlistManage", data: {action: "ADD", tracks: [this.props.track]}});
	}

	playlistRemove() {
		this.props.events.publish({event: "playlistManage", data: {action: "REMOVE", tracks: [this.props.track], id: this.props.playlist}});
	}

	render() {
		var playlistButton;
		if (this.props.playlist) {
			playlistButton = (
				<button className="ui mini compact icon teal button" title="Remove from playlist" onClick={this.playlistRemove}>
					<i className="minus icon"></i>
				</button>
			);
		} else {
			playlistButton = (
				<button className="ui mini compact icon teal button" title="Add to playlist" onClick={this.playlistAdd}>
					<i className="list icon"></i>
				</button>
			);
		}

		return (
			<tr className={this.props.playing ? "positive" : ""}>
				<td>
					<button className="ui mini compact icon green button" onClick={this.play} title="Play now"><i className="play icon"></i></button>
					<button className="ui mini compact icon olive button" onClick={this.enqueue} title={this.props.queued ? "Remove from queue" : "Add to queue"}>
						<i className={this.props.queued ? "minus icon" : "plus icon"}></i>
					</button>
					{playlistButton}
				</td>
				<td>
					{this.props.track.discNumber ? (this.props.track.discNumber + '.' + this.props.track.track) : this.props.track.track}
				</td>
				<td>
					{this.props.track.artist}
				</td>
				<td>
					{this.props.track.title}
				</td>
				<td>
					{/* <CoverArt subsonic={this.props.subsonic} id={this.props.track.coverArt} size={this.props.iconSize} /> */}
					{this.props.track.album}
				</td>
				<td>
					{this.props.track.year}
				</td>
				<td className="right aligned">
					{this.props.track.duration ? SecondsToTime(this.props.track.duration) : '?:??'}
				</td>
			</tr>
		);
	}
}
