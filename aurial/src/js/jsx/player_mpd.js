import {h, Component} from 'preact';
import AudioPlayer from '../audioplayer'
import {SecondsToTime, ArrayShuffle} from '../util'
import {CoverArt} from './common'
import {Messages} from './app'
import SoapClient from '../soaprequest.js';
const socket = new WebSocket('ws://localhost:8000/ws');

export default class Player extends Component {
	noImage = 'css/aurial_200.png';

	static defaultProps = {
		trackBuffer: false
	}

	playerQueue = [];
	buffered = false;
	player = null;
	queue = []; // the queue we use internally for jumping between tracks, shuffling, etc
	/*mpdstate
	{"type":"state", "data":{ "state":2, "volume":55, "repeat":0, "single":0, "crossfade":0, "consume":0, "random":0,  "songpos": 1, "elapsedTime": 2, "totalTime":195,  "currentsongid": 2}}
	state:1-stop;2-playing;3-pause
	repeat:是否开启重复播放模式
	single:开启单曲循环模式
	crossfade:是否开启交叉淡入淡出
	consume:是否开启消费模式
	random:是否开启随机播放
	songpos:歌曲在队列中的位置
	elapsedTime：当前播放歌曲已经播放的时间，单位为秒
	totalTime：当前播放歌曲的总时长，单位为秒
	currentsongid：当前播放歌曲的唯一标识符（ID）
	*/
	state = {
		mpdstate:null
	}
	
	constructor(props, context) {
		super(props, context);
		props.events.subscribe({
			subscriber: this,
			event: ["playerPlay", "playerToggle", "playerStop", "playerNext", "playerPrevious", "playerEnqueue","playerVolume"]
		});

		if (props.persist === true) {
			// this is (badly) delayed to allow it a chance to set up, and stuff. this is a bad idea
			setTimeout(function() {
				props.events.publish({event: "playerEnqueue", data: {action: "ADD", tracks: JSON.parse(localStorage.getItem('queue'))}});
			}, 500);
		}
	}

	componentWillUpdate(nextProps, nextState) {
		if (this.queueDiff(this.queue, nextState.queue) || this.state.shuffle !== nextState.shuffle) {
			this.queue = (this.state.shuffle || nextState.shuffle) ? ArrayShuffle(nextState.queue.slice()) : nextState.queue.slice();
		}
	}
	componentDidMount() {
        // 监听连接建立事件
        socket.addEventListener('open', (event) => {
			console.log('ws:Connected to the server');
			//socket.send('Hello, server!');
		});
		// 监听接收到消息事件
		socket.addEventListener('message', (event) => {
			console.log(`ws:Received message from server: ${event.data}`);
			var mpdstate = JSON.parse(event.data);
			if (mpdstate.type == 'state'){
				console.log("mpdstate:"+JSON.stringify(mpdstate.data));
				this.setState({mpdstate:mpdstate});
			}
		});

		// 监听连接关闭事件
		socket.addEventListener('close', (event) => {
			console.log('ws:Connection closed');
		});

		// 监听错误事件
		socket.addEventListener('error', (event) => {
			console.error('ws:WebSocket error:', event);
		});
	}
	componentWillUnmount() {
		// 组件卸载时清理事件监听器
		socket.removeEventListener('open', () => {});
		socket.removeEventListener('message', () => {});
		socket.removeEventListener('close', () => {});
		socket.removeEventListener('error', () => {});
    }

	receive(event) {
		switch (event.event) {
			case "playerPlay": this.play({track: event.data}); break;
			case "playerToggle": this.togglePlay(); break;
			case "playerStop": this.stop(); break;
			case "playerNext": this.next(); break;
			case "playerPrevious": this.previous(); break;
			case "playerEnqueue": this.enqueue(event.data.action, event.data.tracks); break;
			case "playerShuffle": this.setState({shuffle: event.data}); break;
			case "playerVolume": this.volume(event.data); break;
		}
	}

	createPlayer(track) {
		var events = this.props.events;

		var streamUrl = this.props.subsonic.getStreamUrl({id: track.id});

		return new AudioPlayer({
			url: streamUrl,
			volume: this.state.volume,
			onPlay: function() {
				events.publish({event: "playerStarted", data: track});
			},
			onResume: function() {
				events.publish({event: "playerStarted", data: track});
			},
			onStop: function() {
				events.publish({event: "playerStopped", data: track});
			},
			onPause: function() {
				events.publish({event: "playerPaused", data: track});
			},
			onProgress: function(position, duration) {
				events.publish({event: "playerUpdated", data: {track: track, duration: duration, position: position}});

				// at X seconds remaining in the current track, allow the client to begin buffering the next stream
				if (!this.buffered && this.props.trackBuffer > 0 && duration - position < (this.props.trackBuffer * 1000)) {
					var next = this.nextTrack();
					console.log("Prepare next track", next);
					if (next !== null) {
						this.buffered = true;
						this.playerQueue.push({
							track: next,
							player: this.createPlayer(next)
						});
					} else {
						console.log("There is no next track");
					}
				}

			}.bind(this),
			onLoading: function(loaded, total) {
				events.publish({event: "playerLoading", data: {track: track, loaded: loaded, total: total}});
			},
			onComplete: function() {
				events.publish({event: "playerFinished", data: track});
				this.next();
			}.bind(this)
		});
	}

	play(playItem) {
		if(playItem == null){ 
			socket.send('MPD_API_SET_PLAY');//播放
		}else{
			this.stop();//停止当前播放
			var url=this.props.subsonic.getStreamUrl(playItem.track);
			//console.log("url:"+url);
			console.log("track:"+JSON.stringify(playItem.track))
			//socket.send('MPD_API_RM_ALL');//清空队列
			socket.send('MPD_API_ADD_PLAY_TRACK'+','+url+'\0');//添加当前track
			socket.send('MPD_API_SET_PLAY');//播放
		}
	}

	next() {
		socket.send('MPD_API_SET_NEXT');
	}

	previous() {
		socket.send('MPD_API_SET_PREV');
	}

	nextTrack() {
		var next = null;
		if (this.queue.length > 0) {
			var idx = this.state.playing == null ? 0 : Math.max(0, this.queue.indexOf(this.state.playing));

			if (idx < this.queue.length - 1) {
				idx++;
			} else {
				// it's the end of the queue, user may choose to not repeat, in which case return no next track
				if (this.state.playing != null && localStorage.getItem('repeatQueue') === 'false') return null
				else idx = 0;
			}

			next = this.queue[idx];
		}

		return next;
	}

	previousTrack() {
		var previous = null;
		if (this.queue.length > 0) {
			var idx = this.state.playing == null ? 0 : Math.max(0, this.queue.indexOf(this.state.playing));

			if (idx > 0) idx--;
			else idx = this.queue.length - 1;

			previous = this.queue[idx];
		}

		return previous;
	}

	togglePlay() {
		if (this.player != null) {
			this.player.togglePause();
		} else if (this.state.playing != null) {
			this.play({track: this.state.playing});
		} else if (this.queue.length > 0) {
			this.next();
		}
	}

	stop() {
		socket.send('MPD_API_SET_STOP');
	}

	volume(volume) {
		socket.send('MPD_API_SET_VOLUME'+','+volume);
	}

	enqueue(action, tracks) {
		var queue = this.state.queue.slice();

		if (action === "REPLACE") {
			queue = tracks.slice();
			Messages.message(this.props.events, "Added " + tracks.length + " tracks to queue.", "info", "info");
		} else if (action === "ADD") {
			var trackIds = queue.map(function(t) {
				return t.id;
			});

			var added = 0;
			var removed = 0;
			for (var i = 0; i < tracks.length; i++) {
				var idx = trackIds.indexOf(tracks[i].id);
				if (idx === -1) {
					queue.push(tracks[i]);
					trackIds.push(tracks[i].id);
					added ++;
				} else {
					queue.splice(idx, 1);
					trackIds.splice(idx, 1);
					removed ++;
				}
			}
			if (tracks.length === 1) {
				var trackTitle = tracks[0].artist + " - " + tracks[0].title;
				Messages.message(this.props.events, (added ? "Added " + trackTitle + " to queue. " : "") + (removed ? "Removed " + trackTitle + " from queue." : ""),	"info", "info");
			} else if (added || removed) {
				Messages.message(this.props.events, (added ? "Added " + added + " tracks to queue. " : "") + (removed ? "Removed " + removed + " tracks from queue." : ""), "info", "info");
			}
		}
		//调用/api/queue接口同步queue信息给mpd
		const successFunction = function(result){
			console.log(result)
		};
		const errorFunction = function(message, transport){
			console.log(message)
		}
		//queue增加url字段
		for(let i =0;i<queue.length;i++){
			//queue[i].url=this.props.subsonic.url+'/rest/'+this.props.subsonic.getStreamUrl(queue[i].id);
			//queue[i].url=this.props.subsonic.getStreamUrl(queue[i].id);
			//console.log("queue[i].url:" + queue[i].url)
		}
		/*
		const request = new SoapClient("/api/queue/1",queue);
		request.sendRequest(function(result){     
			if (successFunction){
				successFunction(result);
			}
		}, function(message, transport) {
			if (errorFunction) {errorFunction(message, transport);}
		});	
		*/		
		//---end		

		this.setState({queue: queue});

		this.props.events.publish({event: "playerEnqueued", data: queue});

		if (this.props.persist) {
			localStorage.setItem('queue', JSON.stringify(queue));
		}
	}

	queueDiff(q1, q2) {
		if (q1.length !== q2.length) return true;

		var diff = true;

		q1.forEach(function(t1) {
			var found = false;
			for (const t2 of q2) {
				if (t1.id === t2.id) {
					found = true;
					break;
				}
			}
			diff = diff && !found;
		});

		return diff;
	}

	render() {
		var nowPlaying = "Nothing playing";
		var coverArt = <img src={this.noImage} />;

		if (this.state.playing != null) {
			coverArt = <CoverArt subsonic={this.props.subsonic} id={this.state.playing.coverArt} size={80} events={this.props.events} />;
		}

		return (
			<div className="ui basic segment player">
				<div className="ui items">
					<div className="ui item">
						<div className="ui tiny image">
							{coverArt}
						</div>
						<div className="content">
							<div className="header">
								<PlayerPlayingTitle events={this.props.events} playing={this.state.playing} />
							</div>
							<div className="meta">
								<PlayerPlayingInfo events={this.props.events} playing={this.state.playing} />
							</div>
							<div className="description">
								<table><tbody>
									<tr>
										<td className="controls">
											<div className="ui black icon buttons">
												<PlayerPriorButton key="prior" events={this.props.events} />
												<PlayerPlayToggleButton key="play" events={this.props.events} />
												<PlayerStopButton key="stop" events={this.props.events} />
												<PlayerNextButton key="next" events={this.props.events} />
												<PlayerShuffleButton key="shuffle" events={this.props.events} />
												<PlayerPositionDisplay key="time" events={this.props.events} />
											</div>
										</td>
										<td className="progress">
											<PlayerProgress key="progress" events={this.props.events} />
										</td>
										<td className="volume">
											<PlayerVolume key="volume" events={this.props.events} volume={this.state.volume} />
										</td>
									</tr>
								</tbody></table>
							</div>
						</div>
					</div>
				</div>
			</div>
		);
	}
}

class PlayerPlayingTitle extends Component {
	render() {
		return (
			<span>
				{this.props.playing == null ? "Nothing playing" : this.props.playing.title}
			</span>
		);
	}
}

class PlayerPlayingInfo extends Component {
	render() {
		var album = "Nothing playing";
		if (this.props.playing != null) {
			album = this.props.playing.artist + " - " + this.props.playing.album;
			if (this.props.playing.date) album += " (" + this.props.playing.date + ")";
		}

		return (
			<span>
				{album}
			</span>
		);
	}
}

class PlayerPositionDisplay extends Component {
	state = {
		duration: 0,
		position: 0
	}

	constructor(props, context) {
		super(props, context);
		props.events.subscribe({
			subscriber: this,
			event: ["playerUpdated"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "playerUpdated": this.setState({duration: event.data.duration, position: event.data.position}); break;
		}
	}

	render() {
		return (
			<div className="ui disabled labeled icon button">
				<i className="clock icon"></i>
				{SecondsToTime(this.state.position / 1000)}/{SecondsToTime(this.state.duration / 1000)}
			</div>
		);
	}
}

class PlayerProgress extends Component {
	state = {
		playerProgress: 0,
		loadingProgress: 0
	}

	constructor(props, context) {
		super(props, context);
		props.events.subscribe({
			subscriber: this,
			event: ["playerUpdated", "playerLoading", "playerStopped"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "playerUpdated": this.playerUpdate(event.data.track, event.data.duration, event.data.position); break;
			case "playerLoading": this.playerLoading(event.data.track, event.data.loaded, event.data.total); break;
			case "playerStopped": this.playerUpdate(event.data.track, 1, 0); break;
		}
	}

	playerUpdate(playing, length, position) {
		var percent = (position / length) * 100;
		this.setState({playerProgress: percent});
	}

	playerLoading(playing, loaded, total) {
		var percent = (loaded / total) * 100;
		this.setState({loadingProgress: percent});
	}

	render() {
		var playerProgress = {width: this.state.playerProgress + "%"};
		var loadingProgress = {width: this.state.loadingProgress + "%"};
		return (
			<div className="player-progress">
				<div className="ui red progress">
					<i className="clock icon"></i>
					<div className="track bar" style={playerProgress}></div>
					<div className="loading bar" style={loadingProgress}></div>
				</div>
			</div>
		);
	}
}


class PlayerVolume extends Component {

	constructor(props, context) {
		super(props, context);

		this.mouseDown = this.mouseDown.bind(this);
		this.mouseUp = this.mouseUp.bind(this);
		this.mouseMove = this.mouseMove.bind(this);
	}

	componentWillUnmount() {
	}

	mouseDown(event) {
		this.drag = true;
		this.mouseMove(event);
	}

	mouseUp(event) {
		this.drag = false;
	}

	mouseMove(event) {
		if (this.drag) {
			var rect = document.querySelector(".player-volume").getBoundingClientRect();
			var volume = Math.min(1.0, Math.max(0.0, (event.clientX - rect.left) / rect.width));

			this.props.events.publish({event: "playerVolume", data: volume});
		}
	}

	render() {
		var playerVolume = {width: (this.props.volume*100) + "%"};
		return (
			<div className="player-volume" onMouseDown={this.mouseDown} onMouseMove={this.mouseMove} onMouseUp={this.mouseUp}>
				<div className="ui blue progress">
					<i className="volume up icon"></i>
					<div className="bar" style={playerVolume}></div>
				</div>
			</div>
		);
	}
}

class PlayerPlayToggleButton extends Component {
	state = {
		paused: false,
		playing: false,
		enabled: false
	}

	constructor(props, context) {
		super(props, context);

		this.onClick = this.onClick.bind(this);

		props.events.subscribe({
			subscriber: this,
			event: ["playerStarted", "playerStopped", "playerFinished", "playerPaused", "playerEnqueued"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "playerStarted": this.playerStart(event.data); break;
			case "playerStopped":
			case "playerFinished": this.playerFinish(event.data); break;
			case "playerPaused": this.playerPause(event.data); break;
			case "playerEnqueued": this.playerEnqueue(event.data); break;
		}
	}

	playerStart(playing) {
		this.setState({paused: false, playing: true, enabled: true});
	}

	playerFinish(playing) {
		this.setState({paused: false, playing: false});
	}

	playerPause(playing) {
		this.setState({paused: true});
	}

	playerEnqueue(queue) {
		this.setState({enabled: queue.length > 0});
	}

	onClick() {
		this.props.events.publish({event: "playerToggle"});
	}

	render() {
		return (
			<button className={"ui icon button " + (this.state.enabled ? "" : "disabled")} onClick={this.onClick}>
				<i className={this.state.paused || !this.state.playing ? "play icon" : "pause icon"} />
			</button>
		);
	}
}

class PlayerStopButton extends Component {
	state = {
		enabled: false
	}

	constructor(props, context) {
		super(props, context);

		this.onClick = this.onClick.bind(this);

		props.events.subscribe({
			subscriber: this,
			event: ["playerStarted", "playerStopped", "playerFinished"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "playerStarted": this.playerStart(event.data); break;
			case "playerStopped":
			case "playerFinished": this.playerFinish(event.data); break;
		}
	}

	playerStart(playing) {
		this.setState({enabled: true});
	}

	playerFinish(playing) {
		this.setState({enabled: false});
	}

	onClick() {
		this.props.events.publish({event: "playerStop"});
	}

	render() {
		return (
			<button className={"ui icon button " + (this.state.enabled ? "" : "disabled")} onClick={this.onClick}>
				<i className="stop icon" />
			</button>
		);
	}
}

class PlayerNextButton extends Component {
	state = {
		enabled: false
	}

	constructor(props, context) {
		super(props, context);

		this.onClick = this.onClick.bind(this);

		props.events.subscribe({
			subscriber: this,
			event: ["playerEnqueued"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "playerEnqueued": this.setState({enabled: event.data.length > 0}); break;
		}
	}

	onClick() {
		this.props.events.publish({event: "playerNext"});
	}

	render() {
		return (
			<button className={"ui icon button " + (this.state.enabled ? "" : "disabled")} onClick={this.onClick}>
				<i className="fast forward icon" />
			</button>
		);
	}
}

class PlayerPriorButton extends Component {
	state = {
		enabled: false
	}

	constructor(props, context) {
		super(props, context);

		this.onClick = this.onClick.bind(this);

		props.events.subscribe({
			subscriber: this,
			event: ["playerEnqueued"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "playerEnqueued": this.setState({enabled: event.data.length > 0}); break;
		}
	}

	onClick() {
		this.props.events.publish({event: "playerPrevious"});
	}

	render() {
		return (
			<button className={"ui icon button " + (this.state.enabled ? "" : "disabled")} onClick={this.onClick}>
				<i className="fast backward icon" />
			</button>
		);
	}
}

class PlayerShuffleButton extends Component {
	state = {
		shuffle: false
	}

	constructor(props, context) {
		super(props, context);

		this.onClick = this.onClick.bind(this);
	}

	onClick() {
		var shuffle = !this.state.shuffle;
		this.setState({shuffle: shuffle});
		this.props.events.publish({event: "playerShuffle", data: shuffle});
	}

	render() {
		return (
			<button className="ui icon button" onClick={this.onClick}>
				<i className={"random icon " + (this.state.shuffle ? "red" : "")} />
			</button>
		);
	}
}
