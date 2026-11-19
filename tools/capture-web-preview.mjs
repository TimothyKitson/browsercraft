import { spawn } from "node:child_process";
import { setTimeout as delay } from "node:timers/promises";
import { chromium } from "playwright";

const server = spawn("python", ["-m", "http.server", "8080", "--directory", "build-web"], {
  stdio: "ignore",
  windowsHide: true
});
let browser;
try {
  await delay(1200);
  browser = await chromium.launch({
    headless: true,
    args: [
      "--enable-webgl",
      "--ignore-gpu-blocklist",
      "--use-gl=angle",
      "--use-angle=swiftshader",
      "--enable-unsafe-swiftshader"
    ]
  });

  const page = await browser.newPage({
    viewport: { width: 1280, height: 720 },
    deviceScaleFactor: 1
  });
  page.on("pageerror", error => console.error("Game page error:", error.message));
  page.on("console", message => {
    if (message.type() === "error") console.error("Game console:", message.text());
  });

  await page.goto("http://127.0.0.1:8080/?capture=1", {
    waitUntil: "load",
    timeout: 120000
  });
  await page.locator("#play").waitFor({ state: "visible", timeout: 120000 });
  await page.locator("#play").click();
  await delay(65000);

  const overlayHidden = await page.locator("#overlay").evaluate(el => el.classList.contains("hidden"));
  if (!overlayHidden) throw new Error("The game did not leave its loading overlay.");
  await page.screenshot({ path: "browsercraft-updated-hand.png" });
  console.log("Captured browsercraft-updated-hand.png");
} finally {
  if (browser) await browser.close();
  server.kill();
}
