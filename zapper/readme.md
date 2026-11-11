# zapper

How to run this piece of junk:

1. have `ffmpeg` on your path
	1. [https://github.com/FFmpeg-Builds/FFmpeg-Builds](https://github.com/FFmpeg-Builds/FFmpeg-Builds)
	1. `rundll32 sysdm.cpl,EditEnvironmentVariables`
1. also have `tsc` on your path
	1. `npm -g i typescript@latest`
	1. `npm i`
1. double-click that *juicy* `#run.bat` file
1. do the OBS setup steps (below)
1. hit "Start Recording" in OBS
	1. *don't* hit the "Start Streaming" button
	1. I'm serious
1. Smile

## IMPORTANT NOTE

Don't leave the Director idling on a static screen of heavily-lit pixels.

Those pixels get hella steamy. The plastic cover on the sign will probably begin to warp.

---

## OBS setup

You can import the profile in [the `obs_profile` folder](./obs_profile/), or, if that doesn't work, you can set it up manually.

In the **Profile** menu at the top:

- **New...**
	- call it "Zapper"
	- uncheck the "Show auto-configuration wizard" box

In the **Settings** menu somewhere on the screen:

- ADVANCED:
	- color format: **nv12**
	- color space: **sRGB**
	- color range: **full**
- VIDEO:
	- base canvas resolution: **144x84**
	- output scaled resolution: **144x84**
	- integer fps values: **88**
- OUTPUT:
	- output mode: **Advanced** (yes, really)
	- RECORDING tab:
		- type: **custom output (ffmpeg)**
		- ffmpeg output type: **output to url**
		- file path or URL: **tcp://127.0.0.1:39633**
		- container format: **rawvideo (video)**
- And then you're gonna want to make a new scene.
- Also, make a new source.
	- I'm using media source for now, but i think you can do, like, window capture, too.
- Right-click on the source:
	- **transform**
		- stretch to screen (ctrl+s)
	- **filters**
		- effects filters
			- Add
				- **sharpen**
					- crank that slider all the way to the right.

---

## FFmpeg

the `ffmpeg` command that I used to generate the palette (needed a bit of manual editing afterwards):
```
ffmpeg -i pal.mp4 -vf "palettegen=max_colors=4:reserve_transparent=0" palette.png -y
```

the actual `ffmpeg` command that's used in the code:
```
ffmpeg -f rawvideo -pixel_format nv12 -video_size 144x84 -framerate 88 -i tcp://127.0.0.1:39633?listen -i palette.png -filter_complex [0:v][1:v]paletteuse=dither=none -f rawvideo -pix_fmt rgb24 pipe:1
```
