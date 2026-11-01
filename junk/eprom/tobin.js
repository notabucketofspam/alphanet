const fs = require('node:fs');

const eprom_raw_txt = fs.readFileSync('eprom-raw.txt', {encoding: 'utf8'});
const eprom_bytes = eprom_raw_txt.split(/\s/).map(byte => Number.parseInt(byte, 16));
const eprom = Buffer.from(eprom_bytes);
fs.writeFileSync('eprom.bin', eprom);
