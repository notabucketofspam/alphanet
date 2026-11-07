import dgram from "node:dgram";
import { Buffer } from "node:buffer";

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

async function thing_do() {
  for (let i = 0; i < 1680; i++) {
    // tx_buf.writeUInt8(0x02, i);
  }

  console.log("Starting UDP transmission...");
  let bitOffset = 7;
	let byteOffset = 0;
  setInterval(function () {
    // some kinda offset functions (not slop, i promise)
    bitOffset--;
    if (bitOffset < 0) {
      bitOffset = 7;
			tx_buf[byteOffset] = 0;
			byteOffset++;
    }
    if (byteOffset >= tx_buf.length) {
			byteOffset = 0;
    }
    tx_buf[byteOffset] = (1 << bitOffset);
    socket.send(tx_buf, 5005, "espressif.local", (err) => {
      if (err) console.error("Send error:", err);
    });
  }, 200);
}

thing_do();
