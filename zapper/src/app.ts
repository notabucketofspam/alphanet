import dgram from "node:dgram";
import { Buffer } from "node:buffer";
/**
 * THE FRAME FORMAT:
 */
// 14 rows * 120 bytes per row (60 Red + 60 Green) = 1680 bytes
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