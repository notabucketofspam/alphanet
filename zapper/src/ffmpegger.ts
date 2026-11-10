import { spawn, type ChildProcessWithoutNullStreams } from 'child_process';
import { Transform, TransformCallback } from 'stream';

import {
	VID_WIDTH,
	VID_HEIGHT,
	FRAME_SIZE,
} from "./the-casino.js";

/**thanks gemini*/
class FrameSplitter extends Transform {
	private buffer: Buffer;

	constructor() {
		super();
		this.buffer = Buffer.alloc(0);
	}

	_transform(chunk: Buffer, encoding: BufferEncoding, callback: TransformCallback): void {
		this.buffer = Buffer.concat([this.buffer, chunk]);

		while (this.buffer.length >= FRAME_SIZE) {
			const frame = this.buffer.subarray(0, FRAME_SIZE);
			this.buffer = this.buffer.subarray(FRAME_SIZE);
			this.emit('frame', frame);
		}
		callback();
	}
}

/**enable this to realize why your ffmpeg is borked*/
const debug_me_pls = false;

type Shareholder = {
	splitter: FrameSplitter;
	ffmpeg: ChildProcessWithoutNullStreams;
};

export function init_ffmpeg(): Shareholder {
	const shareholder = {
		splitter: new FrameSplitter(),
		ffmpeg: birth_of_ffmpeg()
	};

	function ffmpeg_close(ffmpeg: ChildProcessWithoutNullStreams, code: number | null) {
		ffmpeg.stdout.unpipe(shareholder.splitter);
		if (shareholder.ffmpeg !== ffmpeg) {
			return;
		}

		console.log(`ffmpeg: exit ${code}`);
		shareholder.ffmpeg = birth_of_ffmpeg();
		ffmpeg_init(shareholder.ffmpeg);
	}
	function ffmpeg_init(ffmpeg: ChildProcessWithoutNullStreams) {
		ffmpeg.stdout.pipe(shareholder.splitter, { end: false });
		ffmpeg.once('close', (code) => ffmpeg_close(ffmpeg, code));
	}
	ffmpeg_init(shareholder.ffmpeg);

	console.log('Ok cool. Now go into OBS and hit the "Start Recording" button.');
	// console.log('Don\'t hit the "Start Streaming" button, though.');

	return shareholder;
}

function birth_of_ffmpeg(): ChildProcessWithoutNullStreams {
	// im assuming here that ffmpeg is on your path.
	// if it isn't, then put it on your path and restart visual studio and/or command prompt.
	const ffmpeg = spawn('ffmpeg', [
		// 1. you get somevideo
		'-f', 'rawvideo',
		'-pixel_format', 'nv12',
		'-video_size', `${VID_WIDTH}x${VID_HEIGHT}`,
		'-framerate', '88',
		'-i', 'tcp://127.0.0.1:39633?listen',

		// 2. apply that palette
		'-i', 'palette.png',
		'-filter_complex', '[0:v][1:v]paletteuse=dither=none',

		// 3. give this man the corporation
		'-f', 'rawvideo',
		'-pix_fmt', 'rgb24',
		'pipe:1'
	]);
	if (debug_me_pls) {
		ffmpeg.stderr.on('data', (data: Buffer) => {
			console.error(data.toString("utf8"));
		});
	}
	return ffmpeg;
}
