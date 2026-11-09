import { Buffer } from "node:buffer";

import { bitmapRowToOffset } from "./alpha-row.js";
import { maskImageBitmap } from "./screen-mask.js";


function writeBitmapRowToBuffer(buffer:Buffer, offset: number, rowdat: Buffer) {
	if (rowdat.length !== 120) {
		throw new RangeError("row length needs to be 120");
	}
	buffer.fill(0, offset, (offset+15));
	let byteIndex = offset;
	for (let col = 0; col < rowdat.length; ++col) {
		const theBit = 0x80 >> (col % 0x08);
		buffer[byteIndex] |= (theBit * rowdat[col]);
		if (theBit & 1) {
			++byteIndex;
		}
	}
}

/**
 * writes image data to a buffer
 * @param buffer the 1680-byte data buffer
 * @param imgdat the 143x84 image bitmap (arranged as R1C1)
 * @param isGreen tell you which color this is
 */
export function writeImageBitmapToBuffer(buffer: Buffer, imgdat: Buffer[], isGreen: boolean) {
	const m_img = maskImageBitmap(imgdat);
	for (let row = 0; row < m_img.length; ++row) {
		const offset = bitmapRowToOffset(row, isGreen);
		writeBitmapRowToBuffer(buffer, offset, m_img[row]);
	}
}
