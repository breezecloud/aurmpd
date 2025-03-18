import {h, Component} from 'preact';
import AudioPlayer from '../audioplayer'
import {SecondsToTime, ArrayShuffle} from '../util'
import {CoverArt} from './common' 
import {Messages} from './app'

const socket = new WebSocket('ws://localhost:8600/ws');
const fetch = require('node-fetch');

export default class Player extends Component {
	noImage = 'css/aurial_200.png';

	static defaultProps = {
		trackBuffer: false
	}

	player = null;
	queue = []; // the queue we use internally for jumping between tracks, shuffling, etc
	state = {
		mpdstate:null,
		volume: 1.0,
		playing:null,
		album: null
	}

	constructor(props, context) {
		super(props, context);
		props.events.subscribe({
			subscriber: this,
			event: ["playerToggle", "playerStop", "playerNext", "playerPrevious", "playerEnqueue", "playerVolume","songchange","playtrack","browserSelected"]
		});

	}

	componentWillUpdate(nextProps, nextState) {
	}
	componentDidMount() {
        // 监听连接建立事件
        socket.addEventListener('open', (event) => {
			console.log('ws:Connected to the server');
			//socket.send('Hello, server!');
		});
		// 监听接收到消息事件
		socket.addEventListener('message', (event) => {
			console.log(`ws:Received message from server: ${event.data}`);//for debug
			try {
				var mpdrespond = JSON.parse(event.data);
			} catch (error) {
				// 处理解析错误
				console.error("JSON解析失败:", error);
			}
			if (mpdrespond.type == 'state'){
				this.props.events.publish({event: "mpdstatus",data:mpdrespond.data});
				this.setState({mpdstate:mpdrespond});
			}else if(mpdrespond.type == 'update_queue'){
				console.log('queue'+JSON.stringify(mpdrespond.data));
				this.props.events.publish({event: "playerEnqueued"});
			}else if(mpdrespond.type =='song_change'){
				//暂时没用，因为在只启动浏览器情况下不会触发song_change，当前播放队列就无法显示,而是通过tracklist记录播放歌曲id，发送songchange来实现改变歌曲
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

			case "playerToggle": this.togglePlay(); break;
			case "playerStop": this.stop(); break;
			case "playerNext": this.next(); break;
			case "playerPrevious": this.previous(); break;
			case "playerEnqueue": this.enqueue(event.data.action, event.data.tracks); break;
			case "playerVolume": this.volume(event.data); break;
			case "songchange":this.setState({playing:event.data});break;
			case "playtrack":socket.send('MPD_API_PLAY_TRACK,' + event.data.queue_sid); break;
			case "browserSelected": this.setState({album: event.data.tracks}); break;
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
		if (this.state.mpdstate.data.state == 2)
			socket.send('MPD_API_SET_PAUSE');
		else
			socket.send('MPD_API_SET_PLAY');
	}

	stop() {
		socket.send('MPD_API_SET_STOP');
	}

	volume(volume) {
		this.setState({volume: volume});
		var volume = volume*100;
		if(volume >= 99)
			volume = 100;
		if(volume <= 1)
			volume = 0;
		socket.send('MPD_API_SET_VOLUME,'+Math.floor(volume).toString()+' ')
	}

	enqueue(action, tracks) {
		//track添加至mpd
		if(action === 'REPLACE'){
			for(let i =0;i<tracks.length;i++){
				tracks[i].url=this.props.subsonic.getStreamUrl(tracks[i]);
			}			
			fetch("/api/queue/replace/play",{ //replace the queue and play
				method: 'POST',
				headers: {
					'Content-Type': 'application/json',
					'Accept': 'application/json',
				},
				body: JSON.stringify(tracks)
			})
			.then(response =>{
				if (!response.ok) {
					throw new Error(`HTTP request failed,status: ${response.status}`);
				}
				return response.json();//注意：返回的是JavaScript 对象
			})
			.then((data) => {
				//实际不用返回tracks
				this.props.events.publish({event: "playerVolume", data: volume});
				
			})
			.catch(error => {
				console.error('request error:', error.message);
			});
		}

		if(action === 'ADD'){
			for(let i =0;i<tracks.length;i++){
				tracks[i].url=this.props.subsonic.getStreamUrl(tracks[i]);
			}			
			fetch("/api/queue/add/",{ //add tu queue 
				method: 'POST',
				headers: {
					'Content-Type': 'application/json',
					'Accept': 'application/json',
				},
				body: JSON.stringify(tracks)
			})
			.then(response =>{
				if (!response.ok) {
					throw new Error(`HTTP request failed,status: ${response.status}`);
				}
				return response.json();//注意：返回的是JavaScript 对象
			})
			.then((data) => {
				//实际不用返回tracks
			})
			.catch(error => {
				console.error('request error:', error.message);
			});
		}

		if(action === 'ADDPLAY'){ // add to queue and play
			for(let i =0;i<tracks.length;i++){
				tracks[i].url=this.props.subsonic.getStreamUrl(tracks[i]);
			}			
			fetch("/api/queue/add/play",{
				method: 'POST',
				headers: {
					'Content-Type': 'application/json',
					'Accept': 'application/json',
				},
				body: JSON.stringify(tracks)
			})
			.then(response =>{
				if (!response.ok) {
					throw new Error(`HTTP request failed,status: ${response.status}`);
				}
				return response.json();//注意：返回的是JavaScript 对象
			})
			.then((data) => {
				//实际不用返回tracks
			})
			.catch(error => {
				console.error('request error:', error.message);
			});
		}

		if(action === 'DEL'){ // remove a track from queue
			fetch("/api/queue/del",{
				method: 'POST',
				headers: {
					'Content-Type': 'application/json',
					'Accept': 'application/json',
				},
				body: JSON.stringify(tracks)
			})
			.then(response =>{
				if (!response.ok) {
					throw new Error(`HTTP request failed,status: ${response.status}`);
				}
				return response.json();//注意：返回的是JavaScript 对象
			})
			.then((data) => {
				//实际不用返回tracks
			})
			.catch(error => {
				console.error('request error:', error.message);
			});
		}
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
												<PlayerPositionDisplay key="time" events={this.props.events} playing={this.state.playing} />
											</div>
										</td>
										<td className="progress">
											<PlayerProgress key="progress" events={this.props.events} playing={this.state.playing}/>
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
			event: ["mpdstatus"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "mpdstatus":this.setState({duration: event.data.elapsedTime, position: event.data.totalTime});break;
		}
	}

	render() {
		//有时候status的totalTime为0，所以优先playing的时间
		var totalTime = this.props.playing == null ? this.state.position : this.props.playing.duration;
		return (
			<div className="ui disabled labeled icon button">
				<i className="clock icon"></i>
				{SecondsToTime(totalTime)}/{SecondsToTime(this.state.duration )}
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
			event: ["mpdstatus"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "mpdstatus": this.mpdstatus(event.data); break;
		}
	}

	mpdstatus(mpds) {
		//有时候status的totalTime为0，所以优先playing的时间
		var totalTime = this.props.playing == null ? this.state.position : this.props.playing.duration;
		var percent = totalTime == 0 ? 0:(mpds.elapsedTime / totalTime) * 100;
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
			event: ["mpdstatus"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "mpdstatus":this.mpdstatus(event.data);break;
		}
	}

	mpdstatus(mpds){
	var playing=false;var paused=false;var enabled = false;
	if(mpds.state == 2)//play
		playing = true;
	else if(mpds.state == 3)//pause
		paused = true;
	else if(mpds.state == 1){//stop
		playing = false;
		paused = false;
	}
	if(mpds.queueLength > 0 )
		enabled = true
	this.setState({paused: paused, playing: playing, enabled: enabled});
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
		//this.setState({enabled: queue.length > 0});
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
			event: ["mpdstatus"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "mpdstatus":this.mpdstatus(event.data);break;
		}
	}

	mpdstatus(mpds){
		var enabled = true;
		if(mpds.state == 1) //stop
			enabled = false
		this.setState({enabled: enabled});
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
			event: [,"mpdstatus"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "mpdstatus":this.mpdstatus(event.data);break;
		}
	}

	mpdstatus(mpds){
		var enabled = false;
		if(mpds.queueLength > 0)
			enabled = true
		this.setState({enabled: enabled});
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
			event: ["mpdstatus"]
		});
	}

	componentWillUnmount() {
	}

	receive(event) {
		switch (event.event) {
			case "mpdstatus":this.mpdstatus(event.data);break;
		}
	}

	mpdstatus(mpds){
		var enabled = false;
		if(mpds.queueLength > 0)
			enabled = true
		this.setState({enabled: enabled});
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
		props.events.subscribe({
			subscriber: this,
			event: ["mpdstatus"]
		});

	}

	receive(event) {
		switch (event.event) {
			case "mpdstatus":this.mpdstatus(event.data);break;
		}
	}	

	mpdstatus(mpds){
		var enabled = false;
		if((mpds.random == 0)&&(this.state.shuffle == true))
			this.setState({shuffle: false});
		if((mpds.random == 1)&&(this.state.shuffle == false))
			this.setState({shuffle: true});
	}		

	onClick() {
		if(this.state.shuffle == false)
			socket.send("MPD_API_TOGGLE_RANDOM,1");
		else
			socket.send("MPD_API_TOGGLE_RANDOM,0");
	}

	render() {
		return (
			<button className="ui icon button" onClick={this.onClick}>
				<i className={"random icon " + (this.state.shuffle ? "red" : "")} />
			</button>
		);
	}
}
