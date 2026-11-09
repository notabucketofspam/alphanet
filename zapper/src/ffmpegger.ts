import { spawn, type ChildProcessWithoutNullStreams } from 'child_process';
import { Transform, TransformCallback } from 'stream';

const WIDTH: number = 143;
const HEIGHT: number = 84;
const FRAME_SIZE: number = WIDTH * HEIGHT * 3;

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

export function init_ffmpeg():Shareholder {
  const splitter = new FrameSplitter();
  const ffmpeg = birth_of_ffmpeg();

  ffmpeg.stdout.pipe(splitter);

  if (debug_me_pls) {
    ffmpeg.stderr.on('data', (data: Buffer) => {
      console.error(data.toString("utf8"));
    });
  }

  ffmpeg.once('close', (code: number) => {
    console.log(`ffmpeg: exit ${code}`);
  });

  const shareholder = {
    splitter,
    ffmpeg
  };

  console.log('Ok cool. Now go into OBS and hit the "Start Recording" button.');
  console.log('Don\'t hit the "Start Streaming" button, though.');

  return shareholder;
}

function birth_of_ffmpeg(): ChildProcessWithoutNullStreams {
  // im assuming here that ffmpeg is on your path.
  // if it isn't, then put it on your path and restart visual studio and/or command prompt.
  const ffmpeg = spawn('ffmpeg', [
    // 1. you get somevideo
    '-f', 'rawvideo',
    '-pixel_format', 'nv12',
    '-video_size', `${WIDTH}x${HEIGHT}`,
    '-framerate', '88',
    '-i', 'tcp://127.0.0.1:39633?listen',

    // 2. apply that palette
    '-i', 'palette.png',
    '-filter_complex', '[0:v][1:v]paletteuse=dither=bayer',

    // 3. give this man the corporation
    '-f', 'rawvideo',
    '-pix_fmt', 'rgb24',
    'pipe:1'
  ]);
  return ffmpeg;
}

/*
Some important notes regarding the OBS setup:
ADVANCED:
- color format: nv12
- color space: sRGB
- color range: full
VIDEO:
- base canvas resolution: 143x84
- output scaled resolution: 143x84
- integer fps values: 88
OUTPUT:
- output mode: Advanced (yes, really)
- RECORDING tab:
  - type: custom output (ffmpeg)
  - ffmpeg output type: output to url
  - file path or URL: tcp://127.0.0.1:39633
  - container format: rawvideo (video)
And then you're gonna want to make a new scene.
Also, make a new source.
  - I'm using media source for now, but i think you can do, like, window capture, too.
Right-click on the source:
- transform
  - stretch to screen (ctrl+s)
- filters
  - effects filters
    - sharpen
      - crank that slider all the way to the right.
*/
