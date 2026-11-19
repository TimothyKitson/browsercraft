import { spawn } from "node:child_process";
import { mkdir } from "node:fs/promises";
import { setTimeout as delay } from "node:timers/promises";
import { chromium } from "playwright";

const outputDir = "browsercraft-ui-screenshots";
await mkdir(outputDir, { recursive: true });

const server = spawn("python", ["-m", "http.server", "8080", "--directory", "build-web"], {
  stdio: "ignore",
  windowsHide: true
});
let browser;

async function openGamePage(url) {
  const page = await browser.newPage({
    viewport: { width: 1280, height: 720 },
    deviceScaleFactor: 1
  });
  page.on("pageerror", error => console.error("Game page error:", error.message));
  page.on("console", message => {
    if (message.type() === "error") console.error("Game console:", message.text());
  });
  await page.goto(url, { waitUntil: "load", timeout: 120000 });
  await page.locator("#play").waitFor({ state: "visible", timeout: 120000 });

  // The page-wide loading overlay occasionally intercepts a real mouse
  // hit-test even though PLAY is visible. Calling the actual button's
  // click method dispatches its normal DOM click handler from this
  // trusted capture harness, which hides the overlay and focuses canvas.
  await page.locator("#play").evaluate(button => button.click());
  await page.waitForFunction(
    () => document.getElementById("overlay")?.classList.contains("hidden"),
    null,
    { timeout: 15000 }
  );
  await delay(1200);
  return page;
}

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

  // Independent real game launches ensure each menu is drawn in its
  // normal context, with the same renderer and Minecraft GUI atlas.
  const menus = [
    ["01-title.png", "title"],
    ["02-options.png", "settings"],
    ["03-video-settings.png", "video"],
    ["04-controls.png", "controls"],
    ["05-sounds.png", "sounds"],
    ["06-chat-settings.png", "chatsettings"],
    ["07-accessibility.png", "accessibility"],
    ["08-language.png", "language"],
    ["09-singleplayer-worlds.png", "worlds"],
    ["10-create-world.png", "createworld"],
    ["11-multiplayer.png", "multiplayer"],
    ["12-resource-packs.png", "packs"]
  ];

  for (const [filename, screen] of menus) {
    const page = await openGamePage("http://127.0.0.1:8080/?screen=" + encodeURIComponent(screen));
    await page.screenshot({ path: `${outputDir}/${filename}` });
    console.log("Captured UI:", screen);
    await page.close();
  }

  // A new Creative world shows the first-person arm and hotbar in normal play.
  const game = await openGamePage("http://127.0.0.1:8080/?capture=1");
  await delay(65000);
  await game.locator("#canvas").focus();
  await game.screenshot({ path: "browsercraft-updated-hand.png" });
  console.log("Captured first-person hand");

  // The dedicated tilde key should open the Minecraft-style pause menu.
  await game.keyboard.press("Backquote");
  await delay(900);
  await game.screenshot({ path: `${outputDir}/13-pause-menu.png` });
  console.log("Captured pause menu");

  // Escape returns to the world; E opens inventory.
  await game.keyboard.press("Escape");
  await delay(500);
  await game.keyboard.press("e");
  await delay(900);
  await game.screenshot({ path: `${outputDir}/14-inventory.png` });
  console.log("Captured inventory");

  // T opens chat, another useful in-game overlay.
  await game.keyboard.press("Escape");
  await delay(300);
  await game.keyboard.press("t");
  await delay(500);
  await game.screenshot({ path: `${outputDir}/15-chat.png` });
  console.log("Captured chat");

  await game.close();
} finally {
  if (browser) await browser.close();
  server.kill();
}
