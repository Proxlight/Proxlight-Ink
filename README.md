# Proxlight Ink — XIAO ePaper EE05 e-reader (v2.9)

## Upload the firmware
1. Install the **Seeed_GFX** library (same as before). Keep `driver.h` next to `ereader.ino`.
2. Arduino IDE → Tools: **Board:** XIAO_ESP32S3 · **PSRAM:** OPI PSRAM · **Partition Scheme:** "8M with spiffs (3MB APP/1.5MB SPIFFS)".
3. Open `ereader/ereader.ino`, upload.

## Use it (no router needed)
1. Join Wi-Fi **Proxlight-Ink**, password **proxlight123** (change both at the top of `ereader.ino`).
2. Open **http://192.168.4.1** (or http://proxlight.local). If your phone says "no internet", choose *stay connected*.
3. **Choose EPUB** → wait for conversion → check the preview → **Send to e-reader**.
   The reader deletes the previous book (text + pictures), formats the new one, then shows page 1.

Buttons: KEY1 prev (hold: prev chapter) · KEY2 text size (hold: Wi-Fi info) · KEY3 next (hold: next chapter).

## What changed
- Own Wi-Fi access point instead of joining a router; Wi-Fi settings removed.
- Branding: "Proxlight" in the screen footer, "Proxlight Ink" on the website and info screens.
- Battery %% in the footer (small box + number), updated only when a page is drawn. "Clear books & cache" button on the web page.
- EPUB pictures (incl. covers, SVG wrappers) are scaled and dithered to 1-bit in the browser and drawn on the screen.
- Real serif fonts with proper word wrap, centered bold headings, indented paragraphs, quotes, lists, centered captions.
- Everything is laid out before the first page is shown; the web page previews the formatted book first.
- New book upload wipes the old `book.txt` and all `i*.bin` pictures first.

## Notes
- Pictures use flash space (~35 KB each at full screen size). If the reader is full, extra pictures are skipped and you're told.
- The e-reader font is ASCII; accented letters are simplified (é→e) and other scripts are dropped.
- Edit `web/index.html`, then run `python3 tools/make_page.py` to regenerate `ereader/page.h`.
- Tested: browser conversion in Chromium and page layout in a desktop simulation. Not tested on your physical board — if text is sideways, change `SCREEN_ROTATION`.

## Battery (LiPo)
- Reads the battery through the board's divider: ADC **GPIO1 (A0)**, enable **GPIO6 (D5)**, ratio 2.0 (taken from Seeed's EE0x docs; edit `BAT_*` at the top of `ereader.ino` if your readings are off, `BAT_CAL` fine-tunes against a multimeter).
- Shown only when a page is drawn, so it never causes an extra screen refresh. The web page shows the last value too.
- Power saving: CPU 80 MHz, lower Wi-Fi power, divider switched on only while measuring, panel sleeps after each refresh, buttons off.
- Below 3.30 V the reader shows "Battery empty" and deep-sleeps to protect the cell. Charge via USB-C, then press reset.
- Clear books & cache formats the reader storage, so free space is identical every time (this also happens automatically before each upload).

### Charging
No charge-status pin is available, so charging is detected from the voltage (>=4.20 V, a +50 mV jump when USB is plugged in, or a steady rise). While charging, a bolt icon is shown beside the battery box and the percentage is corrected for the charger's voltage boost (`BAT_CHG_OFFSET_MV`). The icon updates on the next page turn.

### Battery reading stability
Page turns and Wi-Fi make the voltage sag briefly. The reading uses the 75th percentile of 24 samples (ignores sag), and on battery the shown percentage only falls, by at most 2% per reading, so it no longer jumps down on each page change.

## Power & battery (450 mAh LiPo)
- v2.9 removes the sleep feature (no Sleep button, no auto-sleep). The only automatic power-off left is a safety shutdown below 3.30 V to protect the cell.
- Awake, the Wi-Fi network is the main drain (about 70-100 mA, so roughly 4-5 h on 450 mAh). Nothing but switching Wi-Fi off can change that much. What is tuned: CPU 80 MHz, Wi-Fi power 5 dBm (raise `WIFI_TX_DBM_ENUM` if the phone drops), 20 MHz channel, one phone at a time, battery divider on only while measuring.
- **Arduino IDE: set Tools > PSRAM to "Disabled".** The reader streams the book from flash, so PSRAM is not needed and it costs a few mA.
- **Percent now matches plugged and unplugged:** the charger raises the battery pin voltage; the reader measures that step at every plug/unplug and learns it (`BAT_CHG_OFFSET_MV` is only the start value), and adds back the Wi-Fi load sag (`BAT_LOAD_COMP_MV`). The shown number never jumps: at most 1-3 % per minute, and not at all within 20 s of its last change.
- **Charging icon:** USB host attached -> charging. Wall charger -> voltage jump/rise rules (a sag that recovers is ignored). The bolt is hidden once the battery is full. It updates on the next page draw.
- If the percent is still off, check the Serial Monitor `Battery:` lines and tune `BAT_CAL` against a multimeter.

## v2.8 changes (pictures + charging)
- **Picture size / style / turn** (web page, "Load a book"): Full screen / Large / Medium / Small; Comic (sharp, Atkinson dither) / Photo / Line art; optional 90-degree turn for tall pages (hold the reader sideways). Changing a setting re-converts the chosen book. Size is fixed when the book is converted - the reader cannot zoom a 1-bit picture.
- Full-screen pictures use the whole panel (the footer is hidden on that page). A full-screen page is ~42 KB, so about 25-30 pages fit per book; split comics with `epub_splitter.py -p 25`.
- Pictures are now downscaled in steps and sharpened before dithering, and picture-only EPUBs (comics) are accepted.
- **Charging icon:** USB power is detected from the ESP32-S3 USB frame counter (works with a computer/laptop/phone port). Plain wall chargers send no frames, so for those the old voltage rules still apply. The icon updates on the next page draw. Untested on hardware.

## Can't open the page? (v2.9.1)
1. Join Wi-Fi **Proxlight-Ink** (password `proxlight123`). Only ONE device can be joined at a time: disconnect your laptop first.
2. Open **http://192.168.4.1** (always works). `http://proxlight.local` works on iPhone/Mac/Windows; some Android phones don't support `.local`, use the IP there. Any other address you type is redirected to the reader page.
3. The reader now answers the phone's "is there internet?" checks, so the phone stays on the Proxlight-Ink Wi-Fi instead of switching the browser to mobile data (that was the usual reason the page "stopped working"). If a phone still shows "no internet", choose **Stay connected**, and turn off mobile data once to test.
4. Wi-Fi power is back at 11 dBm (`WIFI_TX_DBM_ENUM`); 5 dBm was too weak for some phones.
