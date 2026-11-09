function alphaRowToOffset(row: number, isGreen: boolean): number {
	const baseAddr = ((row >> 2) * 60);
	const relAddr = ((row % 4) * 15);
	const bankAddr = isGreen ? 0 : 840;
	return baseAddr + relAddr + bankAddr;
}

/**Convert a row address in the bitmap to a buffer offset*/
export function bitmapRowToOffset(row: number, isGreen: boolean): number {
	const alpha_row = alphaRow[row];
	return alphaRowToOffset(alpha_row, isGreen);
}

/**
 * For converting the bitmap row to the alpha director sign row.
 * Note: you still need to do some math to get the byte offset
 * within the packet buffer.
 */
const alphaRow: number[] = [
	32, // row 0
	36,
	40,
	44,
	48,
	52,
	0,
	4,  // row 7
	8,
	12,
	16,
	20,
	24,
	28,
	33, // row 14
	37,
	41,
	45,
	49,
	53,
	1,
	5,  // row 21
	9,
	13,
	17,
	21,
	25,
	29,
	34, // row 28
	38,
	42,
	46,
	50,
	54,
	2,
	6,  // row 35
	10,
	14,
	18,
	22,
	26,
	30,
	35, // row 42
	39,
	43,
	47,
	51,
	55,
	3,
	7,  // row 49
	11,
	15,
	19,
	23,
	27,
	31,
];
