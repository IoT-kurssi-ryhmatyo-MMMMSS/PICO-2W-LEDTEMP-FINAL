// @ts-check
import { test, expect } from "@playwright/test";

test.beforeEach(async ({ page }) => {
  await page.goto("http://localhost:3000");
});

// Post data before all tests
test.beforeAll(async ({ request }) => {
  await request.post("http://localhost:3000/api/sensors/reset");
  await request.post("http://localhost:3000/api/sensors", {
    data: { temperature: 23.5, humidity: 50, led_temp: 24.1 },
  });
});

test("page has correct title", async ({ page }) => {
  await expect(page).toHaveTitle(/Data from sensors/);
});

test("chart is visible", async ({ page }) => {
  const chart = await page.locator("#curve_chart");
  await expect(chart).toBeVisible();
});

test("data table is present", async ({ page }) => {
  const table = await page.locator("#dataTable");
  await expect(table).toBeVisible();
  await expect(table.locator("thead")).toBeVisible();
  await expect(table.locator("tbody")).toBeVisible();
});

test("fan control sliders and button work", async ({ page }) => {
  const minSlider = page.locator("#minTemp");
  const maxSlider = page.locator("#maxTemp");
  const applyBtn = page.locator("#applyFanBtn");
  const status = page.locator("#fanStatus");
  const fanControls = page.locator(".fan-controls");

  await minSlider.fill("22");
  await maxSlider.fill("30");
  await applyBtn.click();

  await expect(status).toContainText(/Command sent|Sending/);
  await expect(fanControls).toContainText("Min: 22°C");
  await expect(fanControls).toContainText("Max: 30°C");
});

test("auto refresh checkbox toggles", async ({ page }) => {
  const autoRefresh = page.locator("#autoRefresh");
  await expect(autoRefresh).toBeChecked();
  await autoRefresh.uncheck();
  await expect(autoRefresh).not.toBeChecked();
});
