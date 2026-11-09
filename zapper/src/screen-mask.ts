/*
Each pixel is 5mm x 5mm
column gap: 1 pixel (5mm)
row gap: 4 pixel (20mm)
ch: 5px * 7px
width: 24ch
height: 8ch
real size: 120px * 56px
virtual size: 143px * 84px
*/

/**
 * takes a 143-bit virtual row data buffer and deletes some
 * columns in it to turn it into a 120-bit real row data buffer
 */
function maskColumnsInRow(rowdat: number[]) {
	if (rowdat.length !== 143) {
		throw new RangeError("it's gotta be 143, choom");
	}
	for (let i = 0; i < rowdat.length; ++i) {
		if (i % 6 > 4) {
			rowdat[i] = NaN;
		}
	}
	rowdat = rowdat.filter(Number.isInteger);
	return rowdat;
}

/**
 * takes an 84-row virtual image data buffer and deletes some
 * of the rows so that it's a 56-row real image data buffer
 * @param imgdat it's rows, columns
 */
function maskRows(imgdat: number[][]) {
	if (imgdat.length !== 84) {
		throw new RangeError("need to have 84 rows");
	}
	for (let i = 0; i < imgdat.length; ++i) {
		if (i % 11 > 6) {
			imgdat[i] = [];
		}
	}
	imgdat = imgdat.filter((row) => row.length > 0);
	return imgdat;
}

/**
 * deletes some data from a 143x84 bitmap, so that now
 * it's a 120x56 bitmap instead :^)
 * @param imgdat rows, columns
 */
export function maskImageBitmap(imgdat: number[][]) {
	return maskRows(imgdat.map(maskColumnsInRow));
}
