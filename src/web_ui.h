#pragma once
#include <Arduino.h>

// Interface web embutida no firmware (servida em "/"). Mantida simples e leve.
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="pt-br">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>GIF Display</title>
<style>
  :root { color-scheme: dark; }
  * { box-sizing: border-box; }
  body { font-family: system-ui, sans-serif; margin: 0; background:#111; color:#eee; }
  header { padding:16px; background:#1b1b1b; border-bottom:1px solid #333; }
  h1 { margin:0; font-size:20px; }
  main { padding:16px; max-width:680px; margin:0 auto; }
  .bar { height:10px; background:#333; border-radius:5px; overflow:hidden; margin:8px 0; }
  .bar > div { height:100%; background:#4caf50; width:0; transition:width .3s; }
  .muted { color:#999; font-size:13px; }
  .drop { border:2px dashed #555; border-radius:10px; padding:24px; text-align:center;
          margin:16px 0; cursor:pointer; transition:.2s; }
  .drop.hover { border-color:#4caf50; background:#1c2a1c; }
  ul { list-style:none; padding:0; margin:0; }
  li { display:flex; align-items:center; gap:10px; padding:10px; border-bottom:1px solid #2a2a2a; }
  li.active { background:#15321a; }
  li .name { flex:1; word-break:break-all; }
  li .size { color:#999; font-size:12px; }
  button { background:#2a2a2a; color:#eee; border:1px solid #444; border-radius:6px;
           padding:6px 10px; cursor:pointer; }
  button:hover { background:#3a3a3a; }
  button.danger { border-color:#a33; }
  .row { display:flex; gap:8px; margin:12px 0; }
  .play { border-color:#2e7d32; }
  .cfg { border:1px solid #333; border-radius:10px; padding:12px; margin:16px 0; }
  .cfg h2 { margin:0 0 8px; font-size:15px; color:#bbb; }
  .cfgrow { display:flex; align-items:center; gap:10px; margin:8px 0; }
  .cfgrow label { width:90px; color:#aaa; font-size:14px; }
  .cfgrow input[type=range] { flex:1; }
  .cfgrow input[type=number] { width:80px; background:#1b1b1b; color:#eee;
           border:1px solid #444; border-radius:6px; padding:4px; }
  select { background:#1b1b1b; color:#eee; border:1px solid #444;
           border-radius:6px; padding:5px; }
</style>
</head>
<body>
<header><h1>🖼️ GIF Display</h1><div class="muted" id="usage">carregando…</div>
<div class="bar"><div id="usebar"></div></div></header>
<main>
  <div class="row">
    <button onclick="api('/api/prev')">⟵ Anterior</button>
    <button onclick="api('/api/next')">Próximo ⟶</button>
  </div>

  <div class="drop" id="drop">
    Arraste um <b>.gif</b> aqui ou clique para enviar<br>
    <span class="muted">tela 240×135 — redimensione antes p/ melhor resultado</span>
    <input type="file" id="file" accept=".gif" hidden multiple>
  </div>
  <div class="bar" id="upwrap" style="display:none"><div id="upbar"></div></div>

  <ul id="list"></ul>

  <div class="cfg">
    <h2>Configuração da tela</h2>
    <div class="cfgrow"><label>Brilho</label>
      <input type="range" id="bright" min="0" max="255"></div>
    <div class="cfgrow"><label>Rotação</label>
      <select id="rot"><option>0</option><option>1</option><option>2</option><option>3</option></select></div>
    <div class="cfgrow"><label>Modo</label>
      <select id="mode" onchange="toggleInt()">
        <option value="lock">Travar no GIF atual</option>
        <option value="cycle">Trocar periodicamente</option>
      </select></div>
    <div class="cfgrow" id="introw"><label>A cada (s)</label>
      <input type="number" id="interval" min="1" value="10"></div>
    <button onclick="applyCfg()">Aplicar configuração</button>
  </div>
</main>
<script>
const $ = s => document.querySelector(s);
function fmt(b){ return b>1048576 ? (b/1048576).toFixed(1)+" MB" : (b/1024).toFixed(0)+" KB"; }

async function api(path){ await fetch(path,{method:'POST'}); setTimeout(refresh,300); }
async function del(name){ if(!confirm("Apagar "+name+"?"))return;
  await fetch('/api/delete?name='+encodeURIComponent(name),{method:'POST'}); refresh(); }
async function play(i){ await fetch('/api/play?index='+i,{method:'POST'}); setTimeout(refresh,300); }

async function refresh(){
  const r = await fetch('/api/gifs'); const d = await r.json();
  $('#usage').textContent = fmt(d.used)+" / "+fmt(d.total)+" usados • "+d.gifs.length+" GIF(s)";
  $('#usebar').style.width = (100*d.used/d.total).toFixed(1)+"%";
  const ul = $('#list'); ul.innerHTML = "";
  d.gifs.forEach((g,i)=>{
    const li = document.createElement('li'); if(i===d.current) li.className='active';
    li.innerHTML = `<span class="name">${g.name}</span>
      <span class="size">${fmt(g.size)}</span>
      <button class="play" onclick="play(${i})">▶</button>
      <button class="danger" onclick="del('${g.name.replace(/'/g,"\\'")}')">🗑</button>`;
    ul.appendChild(li);
  });
}

function upload(files){
  if(!files.length) return;
  const f = files[0];
  const fd = new FormData(); fd.append('gif', f, f.name);
  const xhr = new XMLHttpRequest();
  $('#upwrap').style.display='block';
  xhr.upload.onprogress = e => { if(e.lengthComputable)
    $('#upbar').style.width = (100*e.loaded/e.total).toFixed(0)+"%"; };
  xhr.onload = () => { $('#upbar').style.width="0"; $('#upwrap').style.display='none';
    refresh(); };
  xhr.open('POST','/api/upload'); xhr.send(fd);
}

const drop = $('#drop'), file = $('#file');
drop.onclick = ()=>file.click();
file.onchange = ()=>upload(file.files);
drop.ondragover = e=>{e.preventDefault(); drop.classList.add('hover');};
drop.ondragleave = ()=>drop.classList.remove('hover');
drop.ondrop = e=>{e.preventDefault(); drop.classList.remove('hover'); upload(e.dataTransfer.files);};

function toggleInt(){
  $('#introw').style.display = $('#mode').value==='cycle' ? 'flex' : 'none';
}
async function loadCfg(){
  const c = await (await fetch('/api/config')).json();
  $('#bright').value = c.brightness;
  $('#rot').value = c.rotation;
  const cyc = c.autoAdvanceMs > 0;
  $('#mode').value = cyc ? 'cycle' : 'lock';
  if(cyc) $('#interval').value = Math.max(1, Math.round(c.autoAdvanceMs/1000));
  toggleInt();
}
async function applyCfg(){
  const cyc = $('#mode').value==='cycle';
  const sec = Math.max(1, parseInt($('#interval').value)||10);
  const p = new URLSearchParams({
    brightness: $('#bright').value,
    rotation: $('#rot').value,
    autoAdvanceMs: cyc ? sec*1000 : 0,
    loop: 1
  });
  await fetch('/api/config?'+p.toString(), {method:'POST'});
  setTimeout(refresh, 300);
}

refresh(); loadCfg(); setInterval(refresh, 5000);
</script>
</body>
</html>)HTML";
