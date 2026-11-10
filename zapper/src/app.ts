import dgram from "node:dgram";
import { Buffer } from "node:buffer";

import { init_ffmpeg } from "./ffmpegger.js";
import { writeImageBitmapToBuffer } from "./buffer-mom.js";
import { bitmapRowToOffset } from "./alpha-row.js";

/*
 * THE FRAME FORMAT:
 * so... on this Director, there's 15 bytes per physical row.
 * HOWEVER, because of the row latch thing, data is basically parsed in four-row chunks.
 * - each char is 5 columns and 7 rows
 * - Each board is 8 chars and 2 rows
 * - The sign is 3 boards wide by 4 boards tall
 * - and that makes 24 chars wide by 8 chars tall
 * - or 120 bits wide and 56 bits tall
 * - or 15 bytes wide and 7 bytes tall
 * Because of the daisy-chaining, we write to the same "row" on every board at once,
 * and then we switch to the next row.
 * Also, there's two colors. so we write 60 bytes for the red
 * and then 60 bytes for the green.
 * 
 */

/** 14 rows * 120 bytes per row (60 Red + 60 Green) = 1680 bytes*/
const tx_buf = Buffer.alloc(1680);
const socket = dgram.createSocket("udp4");

function send_udp() {
	socket.send(tx_buf, 5005, "espressif.local", (err) => {
		if (err) console.error("Send error:", err);
	});
}

const VID_WIDTH = 143;
const VID_HEIGHT = 84;


const bitmap = {
	red: Array<Buffer>(VID_HEIGHT),
	green: Array<Buffer>(VID_HEIGHT)
};

const rgbRowlen = VID_WIDTH * 3;

function processFrame(fbuf: Buffer) {
	let rowIndex = 0;
	let columnIndex = 0;
	for (let i = 0; i < fbuf.length; ++i) {
		if (i % 3 === 0) {
			// red
			bitmap.red[rowIndex][columnIndex] = Number(Boolean(fbuf[i]));
		} else if (i % 3 === 1) {
			// green
			bitmap.green[rowIndex][columnIndex] = Number(Boolean(fbuf[i]));
		} else if (i % 3 === 2) {
			// blue. but we don't have blue.
			++columnIndex
		}
		if ((i % rgbRowlen) === (rgbRowlen - 1)) {
			// cr
			columnIndex = 0;
			// lf
			++rowIndex;
		}
	}
}

function something_important() {
	const { splitter, ffmpeg } = init_ffmpeg();
	splitter.on('frame', (frameBuffer: Buffer) => {
		// This buffer has the 4-color frame. So... do something.
		processFrame(frameBuffer);
		// writeImageBitmapToBuffer(tx_buf, bitmap.red, false);
		writeImageBitmapToBuffer(tx_buf, bitmap.green, true);
		// send_udp();
	});
}

function main_real() {
	for (let i = 0; i < VID_HEIGHT; ++i) {
		bitmap.red[i] = Buffer.alloc(VID_WIDTH);
		bitmap.green[i] = Buffer.alloc(VID_WIDTH);
	}
	socket.on('error', err => {
		if (err) console.error(err);
	})
	socket.bind();

	// tx_buf.writeUInt8(0x80);
	// send_udp();
	// thing_do();
	// thing_do_II();
	// test3();
	something_important();
}
main_real();

function test3() {
	let i = 0;
	setInterval(function () {
		const offset = bitmapRowToOffset(i, true);
		tx_buf.writeUInt8(0x80, offset+3);
		send_udp();
		i++;
	}, 10);
}

function thing_do_II() {
	let rowIndex = 0;
	let columnIndex = 0;
	setInterval(function () {
		for (const buf of bitmap.green) {
			buf.fill(0);
		}
		bitmap.green[rowIndex][columnIndex] = 1;
		++columnIndex;
		if (columnIndex >= VID_WIDTH) {
			// cr
			columnIndex = 0;
			// lf
			++rowIndex;
		}
		if (rowIndex >= VID_HEIGHT) {
			rowIndex = 0;
		}
		writeImageBitmapToBuffer(tx_buf, bitmap.green, true);
		send_udp();
	}, 10);
}

async function thing_do() {
	let col = 0;
	let byteIndex = 0;
	setInterval(function () {
		// some kinda offset functions (not slop, i promise)

		tx_buf.fill(0);
		const theBit = 0x80 >> (col % 0x08);
		tx_buf.writeUInt8(theBit, byteIndex);

		send_udp();

		col++;
		if (theBit & 1) {
			++byteIndex;
		}
		if (byteIndex >= tx_buf.length) {
			byteIndex = 0;
			col = 0;
		}
	}, 10);
}
