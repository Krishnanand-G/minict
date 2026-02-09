const rows = document.querySelector("#rows");
const count = document.querySelector("#count");
const source = document.querySelector("#source");

function esc(s) {
  const el = document.createElement("span");
  el.textContent = s == null ? "" : String(s);
  return el.innerHTML;
}

function render(data, label) {
  const cs = data.containers || [];
  count.textContent = cs.length;
  source.textContent = label;

  if (!cs.length) {
    rows.innerHTML =
      '<tr><td colspan="6">nothing recorded yet — try ' +
      "<code>MINICT_SIM=1 ./build/minict run --name demo /bin/sh</code></td></tr>";
    return;
  }

  rows.innerHTML = cs
    .map(
      (c) =>
        `<tr>` +
        `<td>${esc(c.name)}</td>` +
        `<td class="state-${esc(c.state)}">${esc(c.state)}</td>` +
        `<td>${esc(c.memory)}</td>` +
        `<td>${c.cpu ? c.cpu + "%" : "max"}</td>` +
        `<td>${c.latency_ms} ms</td>` +
        `<td>${esc(c.command)}</td>` +
        `</tr>`
    )
    .join("");
}

async function load() {
  try {
    const r = await fetch("http://127.0.0.1:7474/status", { cache: "no-store" });
    if (!r.ok) throw new Error("bad status");
    render(await r.json(), "live :7474");
  } catch (_) {
    const r = await fetch("sample-status.json");
    render(await r.json(), "sample-status.json");
  }
}

document.querySelector("#refresh").addEventListener("click", load);
load();
