/*
Each pixel is 5mm x 5mm
column gap: 1 pixel (5mm)
row gap: 4 pixel (20mm)
ch: 5px * 7px
width: 24ch
height: 8ch
real size: 120px * 56px
virtual size: 144px * 84px
*/

import { VID_HEIGHT, VID_WIDTH } from "./the-casino.js"

/**
 * takes a 144-byte virtual row data buffer and deletes some
 * columns in it to turn it into a 120-byte real row data buffer
 */
function maskColumnsInRow(rowdat: Buffer) {
	if (rowdat.length !== VID_WIDTH) {
		throw new RangeError(`it's gotta be ${VID_WIDTH}, choom`);
	}
	const rowbuf = Buffer.allocUnsafe(120);
	let byteOffset = 0;
	for (let i = 0; i < rowdat.length; ++i) {
		if (i % 6 < 5) {
			rowbuf[byteOffset] = rowdat[i];
			++byteOffset;
		}
	}
	return rowbuf;
}

/**
 * takes an 84-row virtual image data buffer and deletes some
 * of the rows so that it's a 56-row real image data buffer
 * @param imgdat it's rows, columns
 */
function maskRows(imgdat: Buffer[]) {
	if (imgdat.length !== VID_HEIGHT) {
		throw new RangeError(`need to have ${VID_HEIGHT} rows`);
	}
	for (let i = 0; i < imgdat.length; ++i) {
		if (i % 11 > 6) {
			imgdat[i] = Buffer.allocUnsafe(0);
		}
	}
	imgdat = imgdat.filter((row) => row.length > 0);
	return imgdat;
}

/**
 * deletes some data from a 144x84 bitmap, so that now
 * it's a 120x56 bitmap instead :^)
 * @param imgdat rows, columns
 */
export function maskImageBitmap(imgdat: Buffer[]) {
	return maskRows(imgdat.map(maskColumnsInRow));
}
