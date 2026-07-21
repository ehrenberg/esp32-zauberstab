#pragma once

#include <Arduino.h>

// Eingebettete Single-Page-App (App-Feeling, dark, Bottom-Navigation).
// Keine externen Abhaengigkeiten - das Geraet ist ein Offline-AP.
static const char INDEX_HTML[] PROGMEM = R"=====(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<meta name="theme-color" content="#0b0b10">
<title>Zauberstab</title>
<style>
:root{--bg:#0b0b10;--card:#16161f;--card2:#1f1f2b;--acc:#6c5ce7;--acc2:#00d2ff;--txt:#eef0ff;--mut:#8b8ca3;--ok:#22d39a;--warn:#ff5c7a}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;height:100%;overflow:hidden;background:var(--bg);color:var(--txt);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",system-ui,sans-serif}
#app{display:flex;flex-direction:column;height:100%}
header{padding:14px 16px calc(10px + env(safe-area-inset-top)) 16px;display:flex;align-items:center;gap:12px;background:linear-gradient(135deg,#1a1730,#0b0b10)}
header h1{font-size:19px;margin:0;font-weight:700;letter-spacing:.3px;flex:1}
.pill{font-size:12px;padding:6px 11px;border-radius:999px;background:#000;color:var(--mut);border:1px solid #2a2a3a;white-space:nowrap}
.pill.on{color:var(--ok);border-color:#13503b}
.pill.lock{color:var(--acc2);border-color:#0e4a57}
main{flex:1;overflow-y:auto;-webkit-overflow-scrolling:touch;padding:14px 16px 96px}
.view{display:none;animation:f .22s ease}
.view.act{display:block}
@keyframes f{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:none}}
.previewWrap{display:flex;flex-direction:column;align-items:center;margin:2px 0 16px}
#preview{width:min(68vw,260px);aspect-ratio:1;border-radius:50%;background:#000;box-shadow:0 0 0 2px #23233a,0 14px 40px -12px #000}
#previewInfo{margin-top:9px;font-size:11px;color:var(--mut);letter-spacing:.4px}
#previewAnim{margin-top:7px;font-size:11px;color:var(--mut);border:1px solid #2a2a3a;background:#000;border-radius:999px;padding:5px 12px;cursor:pointer}
#previewAnim.on{color:var(--acc2);border-color:#0e4a57}
.hvy{margin-left:5px;opacity:.75;font-size:12px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.tile{background:var(--card);border:1px solid #24243400;border-radius:16px;padding:15px 12px;font-size:15px;font-weight:600;text-align:center;cursor:pointer;transition:.12s;border:1px solid #23233a}
.tile:active{transform:scale(.96)}
.tile.sel{background:linear-gradient(135deg,var(--acc),#4834d4);border-color:transparent;box-shadow:0 8px 20px -8px var(--acc)}
.card{background:var(--card);border:1px solid #23233a;border-radius:18px;padding:16px;margin-bottom:14px}
.card h2{font-size:13px;text-transform:uppercase;letter-spacing:.8px;color:var(--mut);margin:0 0 12px}
label.row{display:flex;align-items:center;justify-content:space-between;gap:12px;margin:13px 0;font-size:15px}
label.row .v{color:var(--acc2);font-variant-numeric:tabular-nums;font-weight:700;min-width:54px;text-align:right}
input[type=range]{-webkit-appearance:none;width:100%;height:30px;background:transparent;touch-action:pan-y}
input[type=range]::-webkit-slider-runnable-track{height:6px;border-radius:6px;background:#2c2c3e}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;border-radius:50%;background:var(--acc);margin-top:-9px;box-shadow:0 2px 8px #0008}
.rng{display:block;margin:4px 0 16px}
select,input[type=text]{width:100%;background:var(--card2);color:var(--txt);border:1px solid #2c2c3e;border-radius:12px;padding:13px;font-size:16px}
.sw{position:relative;width:52px;height:30px}
.sw input{display:none}
.sw i{position:absolute;inset:0;background:#2c2c3e;border-radius:999px;transition:.18s}
.sw i:before{content:"";position:absolute;width:24px;height:24px;border-radius:50%;background:#fff;top:3px;left:3px;transition:.18s}
.sw input:checked + i{background:var(--ok)}
.sw input:checked + i:before{transform:translateX(22px)}
.btn{display:block;width:100%;padding:15px;border:0;border-radius:14px;font-size:16px;font-weight:700;color:#fff;background:var(--acc);cursor:pointer;margin-top:6px}
.btn:active{transform:scale(.98)}
.btn.sec{background:var(--card2);color:var(--txt);border:1px solid #2c2c3e}
.btn.warn{background:var(--warn)}
.btn.ok{background:var(--ok);color:#062a1e}
.btnrow{display:flex;gap:10px}.btnrow .btn{margin-top:0}
.pal{display:flex;flex-wrap:wrap;gap:9px;margin:4px 0}
.sw0{width:34px;height:34px;border-radius:10px;border:2px solid #0000;cursor:pointer}
.sw0.sel{border-color:#fff;transform:scale(1.08)}
.chips{display:flex;gap:8px;margin-bottom:12px}
.chip{flex:1;text-align:center;padding:10px;border-radius:12px;background:var(--card2);border:1px solid #2c2c3e;font-weight:600;cursor:pointer}
.chip.sel{background:var(--acc);border-color:transparent}
#canvas{width:100%;background:#000;border-radius:12px;border:1px solid #23233a;touch-action:none;display:block}
nav{position:fixed;bottom:0;left:0;right:0;display:flex;background:#0d0d14ee;backdrop-filter:blur(12px);border-top:1px solid #1e1e2c;padding-bottom:env(safe-area-inset-bottom)}
nav button{flex:1;background:0;border:0;color:var(--mut);padding:11px 0 13px;font-size:11px;font-weight:600;cursor:pointer;display:flex;flex-direction:column;align-items:center;gap:4px}
nav button.act{color:var(--acc2)}
nav .ic{font-size:21px;line-height:1}
.hint{color:var(--mut);font-size:13px;line-height:1.5}
.info{display:inline-flex;align-items:center;justify-content:center;width:18px;height:18px;border-radius:50%;border:1px solid var(--mut);color:var(--mut);font-size:11px;font-style:normal;font-weight:700;cursor:pointer;margin-left:7px;vertical-align:middle;flex:none}
.modal{position:fixed;inset:0;background:#000b;display:none;align-items:center;justify-content:center;z-index:60;padding:24px;backdrop-filter:blur(3px)}
.modal.open{display:flex}
.modalbox{background:var(--card2);border:1px solid #33334a;border-radius:18px;padding:22px;max-width:360px;width:100%;animation:f .2s ease}
.modalbox h3{margin:0 0 12px;font-size:18px}
.modalbox p{margin:0 0 18px;color:var(--mut);font-size:14px;line-height:1.6}
.kv{display:grid;grid-template-columns:1fr auto;gap:9px 12px;font-size:14px}
.kv .k{color:var(--mut)}
.kv .val{font-variant-numeric:tabular-nums;font-weight:700;text-align:right}
.kv .val.bad{color:var(--warn)}
.kv .val.good{color:var(--ok)}
</style>
</head>
<body>
<div id="app">
<header>
<h1>Zauberstab</h1>
<span id="pill" class="pill">…</span>
<span id="pillStart" class="pill" onclick="App.toggleRun()" style="cursor:pointer">▶</span>
</header>
<main>
<div class="previewWrap"><canvas id="preview"></canvas><div id="previewInfo"></div>
<button id="previewAnim" onclick="App.toggleAnim()">▶ Animation</button></div>

<section id="v-muster" class="view act">
<div id="tiles" class="grid"></div>
</section>

<section id="v-malen" class="view">
<div class="card">
<h2>Slot</h2>
<div id="slots" class="chips"></div>
<canvas id="canvas" width="240" height="325"></canvas>
<div class="pal" id="drawPal"></div>
<div class="btnrow">
<button class="btn sec" onclick="App.editClear()">Leeren</button>
<button class="btn ok" onclick="App.editSave()">Speichern</button>
</div>
<button class="btn" style="margin-top:10px" onclick="App.editShow()">Auf Stab anzeigen</button>
<p class="hint" style="margin-top:12px">Senkrecht = Position auf dem Stab, waagerecht = Drehwinkel. Speichern legt die Zeichnung im Slot ab.</p>
</div>
</section>

<section id="v-text" class="view">
<div class="card">
<h2>Text</h2>
<input type="text" id="txt" maxlength="32" placeholder="Text eingeben…" oninput="App.previewSoon()">
<div class="pal" id="textPal" style="margin-top:12px"></div>
<button class="btn" onclick="App.textApply()">Auf Stab anzeigen</button>
<p class="hint" style="margin-top:12px">Grossbuchstaben, Ziffern und . ! ? - : werden dargestellt. Der Text laeuft einmal pro Umdrehung rund herum.</p>
</div>
</section>

<section id="v-foto" class="view">
<div class="card">
<h2>Foto vom Handy</h2>
<div id="photoSlots" class="chips"></div>
<input type="file" id="photoFile" accept="image/*" style="margin-bottom:12px">
<canvas id="photoSrc" width="256" height="256" style="display:none"></canvas>
<div class="btnrow">
<button class="btn sec" onclick="App.photoClear()">Leeren</button>
<button class="btn ok" onclick="App.photoUpload()">Hochladen</button>
</div>
<button class="btn" style="margin-top:10px" onclick="App.photoShow()">Auf Stab anzeigen</button>
<p class="hint" style="margin-top:12px">Das Bild wird im Telefon rund zugeschnitten und in ein Vollscheiben-Bild umgerechnet. Achtung: ein Foto leuchtet fast alle LEDs gleichzeitig - Helligkeit und Stromlimit moderat halten (Brownout-Schutz).</p>
</div>
</section>

<section id="v-setup" class="view">
<div class="card"><h2>Anzeige</h2><div id="setA"></div></div>
<div class="card"><h2>Bild im Kreis</h2>
<label class="row"><span>Bild oben (statt Vollkreis) <i class="info" onclick="App.info('imode')">i</i></span><span class="sw"><input type="checkbox" id="imode" onchange="App.saveSet('imode',this.checked?1:0)"><i></i></span></label>
<div id="setImg"></div>
<p class="hint">Bildwinkel verschiebt das Bild auf dem Kreis (oben einstellen), Bildhoehe den Abstand zur Mitte, Bildgroesse den Durchmesser. Aus = Muster fuellt den ganzen Kreis.</p>
</div>
<div class="card"><h2>Bewegung &amp; Sensor</h2><div id="setB"></div>
<label class="row"><span>Gyro-Achse <i class="info" onclick="App.info('axis')">i</i></span><select id="axis" onchange="App.saveSet('axis',this.value)"><option value="0">X</option><option value="1">Y</option><option value="2">Z</option></select></label>
<label class="row"><span>Richtung invertieren <i class="info" onclick="App.info('invert')">i</i></span><span class="sw"><input type="checkbox" id="invert" onchange="App.saveSet('invert',this.checked?1:0)"><i></i></span></label>
<label class="row"><span>Phase-Lock (Drift-Korrektur) <i class="info" onclick="App.info('plock')">i</i></span><span class="sw"><input type="checkbox" id="plock" onchange="App.saveSet('plock',this.checked?1:0)"><i></i></span></label>
<button class="btn sec" style="margin-top:8px" onclick="App.calibrate()">Sensor neu kalibrieren</button>
<p class="hint" style="margin-top:10px">Der Gyro-Nullpunkt wird sonst nur beim Einschalten bestimmt. Wandert das Bild langsam, hier neu kalibrieren – Stab dabei still halten.</p>
</div>
<div class="card"><h2>Betrieb</h2>
<div class="btnrow"><button class="btn ok" onclick="App.run(1)">Display starten</button><button class="btn warn" onclick="App.run(0)">Stop</button></div>
<p class="hint" style="margin-top:12px">Beim Start wird das WLAN getrennt (maximale POV-Leistung). Zum Zurueckkehren ins Setup den Taster lang druecken.</p>
</div>
</section>

<section id="v-dev" class="view">
<div class="card"><h2>Drehung (live)</h2><div id="devRot" class="kv"></div></div>
<div class="card"><h2>Sensor (live)</h2><div id="devSensor" class="kv"></div></div>
<div class="card"><h2>Letzte Schleuder-Sitzung</h2><div id="devSession" class="kv"></div>
<p class="hint">Wird beim Display-Start genullt und beim Stop gesichert. errAvg/errMax = Phase-Lock-Winkelfehler pro Umdrehung.</p></div>
<div class="card"><h2>System</h2><div id="devSys" class="kv"></div></div>
</section>
</main>
<nav>
<button data-v="muster" class="act" onclick="App.tab('muster')"><span class="ic">◎</span>Muster</button>
<button data-v="malen" onclick="App.tab('malen')"><span class="ic">✎</span>Malen</button>
<button data-v="text" onclick="App.tab('text')"><span class="ic">A</span>Text</button>
<button data-v="foto" onclick="App.tab('foto')"><span class="ic">🖼</span>Foto</button>
<button data-v="setup" onclick="App.tab('setup')"><span class="ic">⚙</span>Setup</button>
<button data-v="dev" onclick="App.tab('dev')"><span class="ic">⚡</span>DEV</button>
</nav>
<div id="modal" class="modal" onclick="App.closeInfo(event)">
<div class="modalbox" onclick="event.stopPropagation()">
<h3 id="modalTitle"></h3>
<p id="modalText"></p>
<button class="btn" onclick="App.closeInfo()">Verstanden</button>
</div>
</div>
</div>
<script>
const App={
 st:null,pal:[],leds:65,curSlot:0,curColor:2,grid:null,CW:48,_pt:null,curPhotoSlot:0,_frame:null,
 async j(u,o){const r=await fetch(u,o);return r.headers.get('content-type')&&r.headers.get('content-type').includes('json')?r.json():r.text()},
 async init(){
  this.st=await this.j('/api/state');this.pal=this.st.palette;this.leds=this.st.imgRows;
  this.buildTiles();this.buildSlots();this.buildPalettes();this.buildSettings();this.buildPhotoSlots();this.fill();
  this.grid=new Uint8Array(this.CW*this.leds);this.tab('muster');
  await this.loadSlot(this.st.customSlot);this.refresh();
  setInterval(()=>this.poll(),600);
 },
 tab(v){document.querySelectorAll('.view').forEach(e=>e.classList.remove('act'));
  document.getElementById('v-'+v).classList.add('act');
  document.querySelectorAll('nav button').forEach(b=>b.classList.toggle('act',b.dataset.v===v));},
 buildTiles(){const g=document.getElementById('tiles');g.innerHTML='';
  const hv=this.st.heavy||[];
  this.st.patterns.forEach((n,i)=>{const d=document.createElement('div');d.className='tile';d.textContent=n;
   // Blitz = leuchtet viele LEDs gleichzeitig -> Stromlimit dimmt global herunter.
   if(hv[i]){const s=document.createElement('span');s.className='hvy';s.textContent='⚡';d.appendChild(s);}
   d.onclick=()=>this.pick(0,i);g.appendChild(d);});this.markTiles();},
 markTiles(){document.querySelectorAll('#tiles .tile').forEach((t,i)=>
   t.classList.toggle('sel',this.st.patternMode==0&&i==this.st.pattern));},
 buildSlots(){const s=document.getElementById('slots');s.innerHTML='';
  for(let i=0;i<4;i++){const c=document.createElement('div');c.className='chip'+(i==this.curSlot?' sel':'');
   c.textContent='Slot '+(i+1);c.onclick=()=>this.loadSlot(i);s.appendChild(c);}},
 buildPalettes(){const mk=(host,sel,cb)=>{host.innerHTML='';this.pal.forEach((hex,i)=>{const b=document.createElement('div');
   b.className='sw0'+(i==sel?' sel':'');b.style.background=hex;b.onclick=()=>cb(i,host);host.appendChild(b);});};
  mk(document.getElementById('drawPal'),this.curColor,(i,h)=>{this.curColor=i;[...h.children].forEach((c,k)=>c.classList.toggle('sel',k==i));});
  mk(document.getElementById('textPal'),this.st.textColor,(i,h)=>{this.curTextColor=i;[...h.children].forEach((c,k)=>c.classList.toggle('sel',k==i));this.textApply();});
  this.curTextColor=this.st.textColor;},
 SET_A:[['bright','Helligkeit',1,100,1],['columns','POV-Spalten',8,192,1],['blur','Nachleuchten',0,250,1],['persist','Winkelbreite',1,9,1],['current','Stromlimit mA',300,3000,100],['holdus','Halten µs',2000,30000,100]],
 SET_B:[['gain','Angle Gain (Drift-Trim)',0.2,3,0.005],['threshold','Gyro-Schwelle',0.05,8,0.05]],
 SET_IMG:[['iang','Bildwinkel',0,359,1],['irad','Bildhoehe',0,100,1],['iscale','Bildgroesse',5,100,1]],
 buildSettings(){const mk=(host,arr)=>{host.innerHTML='';arr.forEach(([k,lab,mn,mx,st])=>{
   const wrap=document.createElement('div');const float=st<1;
   wrap.innerHTML=`<label class="row"><span>${lab} <i class="info" onclick="App.info('${k}')">i</i></span><span class="v" id="v_${k}"></span></label>`+
    `<input class="rng" type="range" id="s_${k}" min="${mn}" max="${mx}" step="${st}">`;
   host.appendChild(wrap);
   const inp=wrap.querySelector('input');
   inp.oninput=()=>document.getElementById('v_'+k).textContent=float?(+inp.value).toFixed(3):inp.value;
   inp.onchange=()=>this.saveSet(k,inp.value);});};
  mk(document.getElementById('setA'),this.SET_A);mk(document.getElementById('setB'),this.SET_B);
  mk(document.getElementById('setImg'),this.SET_IMG);},
 fill(){const s=this.st.settings;
  [...this.SET_A,...this.SET_B,...this.SET_IMG].forEach(([k,l,mn,mx,st])=>{const e=document.getElementById('s_'+k);if(!e)return;
   e.value=s[k];document.getElementById('v_'+k).textContent=st<1?(+s[k]).toFixed(3):s[k];});
  document.getElementById('axis').value=s.axis;
  document.getElementById('invert').checked=!!s.invert;
  document.getElementById('plock').checked=!!s.plock;
  document.getElementById('imode').checked=!!s.imode;
  document.getElementById('txt').value=this.st.text;},
 async pick(mode,index){this.st.patternMode=mode;if(mode==0)this.st.pattern=index;
  await this.j('/api/select?mode='+mode+'&index='+index,{method:'POST'});this.markTiles();this.refresh();},
 async saveSet(k,v){const b=new URLSearchParams();b.set(k,v);
  // fill() zieht die Regler nach: clampSettings() kann den Wert korrigiert haben.
  this.st=await this.j('/api/settings',{method:'POST',body:b});this.fill();this.refresh();},
 async calibrate(){if(!confirm('Sensor neu kalibrieren?\n\nStab dabei ruhig und still halten.'))return;
  await fetch('/api/calibrate',{method:'POST'});this.flash('Kalibriere…');},
 // ---- Vorschau ----
 previewSoon(){clearTimeout(this._pt);this._pt=setTimeout(()=>this.textApply(),500);},
 // Animierte Muster als Daumenkino: bewusst opt-in, jeder Frame sind ~21 KB
 // ueber den AP und der ESP rendert ihn synchron im Webserver-Handler.
 toggleAnim(){const b=document.getElementById('previewAnim');
  if(this._anim){clearInterval(this._anim);this._anim=null;b.classList.remove('on');b.textContent='▶ Animation';return;}
  b.classList.add('on');b.textContent='■ Animation';
  this._anim=setInterval(()=>{if(!document.hidden)this.refresh();},900);},
 async refresh(){const f=await this.j('/api/frame');this._frame=f;this.draw(f);
  const deg=(f.span*180/Math.PI).toFixed(0);
  document.getElementById('previewInfo').textContent=
   f.span<6.2?f.cols+' Schritte über '+deg+'° · Bildfenster':f.cols+' Spalten · Vollkreis';},
 // Jede Spalte ist ein Kreissegment, kein Punkt: bei 46 Spalten liegen am
 // Aussenrand ~17 px zwischen zwei Spalten - gezeichnete Punkte hinterlassen
 // dort Luecken und das Muster zerfaellt zu Konfetti. Gleichfarbige LEDs
 // untereinander werden zu einem Bogen zusammengefasst (statt 7800 Pfaden).
 draw(f){if(!f)return;
  const cv=document.getElementById('preview'),dpr=Math.min(window.devicePixelRatio||1,3);
  const W=cv.clientWidth||240,px=Math.round(W*dpr);
  if(cv.width!==px){cv.width=px;cv.height=px;}
  const x=cv.getContext('2d');x.setTransform(dpr,0,0,dpr,0,0);
  const cx=W/2,cy=W/2,R=W/2-4;
  x.clearRect(0,0,W,W);
  const raw=atob(f.data),cols=f.cols,leds=f.leds,step=f.span/cols;
  // Winkelbreite wie am Stab: persist verbreitert jede Spalte um Nachbarschritte.
  const hw=step*0.5*Math.max(1,f.persist)*1.08;
  const dr=R/(leds-1);
  for(let c=0;c<cols;c++){
   const a=f.a0+step*(c+0.5);
   let s=-1,cr=0,cg=0,cb=0;
   const flush=e=>{if(s<0)return;
    const r0=Math.max(s*dr-dr*0.5,0),r1=e*dr+dr*0.5,mid=(r0+r1)/2;
    x.strokeStyle='rgb('+cr+','+cg+','+cb+')';x.lineWidth=r1-r0;
    x.beginPath();x.arc(cx,cy,Math.max(mid,0.1),a-hw,a+hw);x.stroke();s=-1;};
   for(let i=0;i<leds;i++){
    const o=(c*leds+i)*2,v=(raw.charCodeAt(o)<<8)|raw.charCodeAt(o+1);
    const r=((v>>11&31)*255/31)|0,g=((v>>5&63)*255/63)|0,b=((v&31)*255/31)|0;
    if(!(r|g|b)){flush(i-1);continue;}
    if(s<0){s=i;cr=r;cg=g;cb=b;}
    else if(r!==cr||g!==cg||b!==cb){flush(i-1);s=i;cr=r;cg=g;cb=b;}}
   flush(leds-1);}
  // Marke bei 12 Uhr - macht den Regler "Bildwinkel" ablesbar (270° = oben).
  x.strokeStyle='#3a3a52';x.lineWidth=2;x.beginPath();
  x.moveTo(cx,cy-R-1);x.lineTo(cx,cy-R+7);x.stroke();},
 // ---- Editor ----
 async loadSlot(i){this.curSlot=i;this.buildSlots();
  const d=await this.j('/api/draw?slot='+i);const raw=atob(d.data);
  this.grid=new Uint8Array(this.CW*this.leds);
  for(let c=0;c<this.CW;c++)for(let r=0;r<this.leds;r++){const cell=c*this.leds+r,by=raw.charCodeAt(cell>>1);
   this.grid[cell]=(cell&1)?(by&15):(by>>4);}
  this.drawEditor();},
 drawEditor(){const cv=document.getElementById('canvas'),x=cv.getContext('2d');
  const cw=cv.width/this.CW,ch=cv.height/this.leds;x.fillStyle='#000';x.fillRect(0,0,cv.width,cv.height);
  for(let c=0;c<this.CW;c++)for(let r=0;r<this.leds;r++){const v=this.grid[c*this.leds+r];if(!v)continue;
   x.fillStyle=this.pal[v];x.fillRect(c*cw,r*ch,Math.ceil(cw),Math.ceil(ch));}},
 paint(e){const cv=document.getElementById('canvas'),b=cv.getBoundingClientRect();
  const t=e.touches?e.touches[0]:e;const c=Math.floor((t.clientX-b.left)/b.width*this.CW);
  const r=Math.floor((t.clientY-b.top)/b.height*this.leds);
  if(c<0||c>=this.CW||r<0||r>=this.leds)return;this.grid[c*this.leds+r]=this.curColor;this.drawEditor();},
 editClear(){this.grid.fill(0);this.drawEditor();},
 pack(){const buf=new Uint8Array(Math.ceil(this.CW*this.leds/2));
  for(let cell=0;cell<this.CW*this.leds;cell++){const v=this.grid[cell]&15;
   if(cell&1)buf[cell>>1]|=v;else buf[cell>>1]|=v<<4;}
  let s='';buf.forEach(b=>s+=String.fromCharCode(b));return btoa(s);},
 async editSave(){await fetch('/api/draw?slot='+this.curSlot,{method:'POST',body:this.pack()});this.flash('Gespeichert');},
 async editShow(){await this.editSave();await this.pick(1,this.curSlot);this.tab('muster');},
 async textApply(){const t=document.getElementById('txt').value;const col=this.curTextColor;
  const b=new URLSearchParams();b.set('text',t);b.set('color',col);
  await this.j('/api/text',{method:'POST',body:b});this.st.patternMode=2;
  await this.j('/api/select?mode=2&index=0',{method:'POST'});this.markTiles();this.refresh();},
 // ---- Foto ----
 buildPhotoSlots(){const s=document.getElementById('photoSlots');if(!s)return;s.innerHTML='';
  const have=this.st.photoSlots||[0,0,0,0];
  have.forEach((v,i)=>{const c=document.createElement('div');
   c.className='chip'+(i==this.curPhotoSlot?' sel':'');
   c.textContent='Foto '+(i+1)+(v?' ●':' ○');
   c.onclick=()=>{this.curPhotoSlot=i;this.buildPhotoSlots();};s.appendChild(c);});},
 async photoUpload(){const f=document.getElementById('photoFile').files[0];
  if(!f){this.flash('Kein Bild gewaehlt');return;}
  this.flash('Verarbeite…');
  const img=await createImageBitmap(f);
  const sq=Math.min(img.width,img.height),sx=(img.width-sq)/2,sy=(img.height-sq)/2,SRC=256;
  const cv=document.getElementById('photoSrc'),x=cv.getContext('2d');
  x.drawImage(img,sx,sy,sq,sq,0,0,SRC,SRC);
  const px=x.getImageData(0,0,SRC,SRC).data;
  const COLS=this.st.imgCols,ROWS=this.st.imgRows,out=new Uint8Array(COLS*ROWS*2);
  for(let c=0;c<COLS;c++){const a=2*Math.PI*c/COLS,ca=Math.cos(a),sa=Math.sin(a);
   for(let r=0;r<ROWS;r++){const rad=r/(ROWS-1),dx=rad*ca,dy=rad*sa;
    let sxp=Math.round((dx*0.5+0.5)*(SRC-1)),syp=Math.round((dy*0.5+0.5)*(SRC-1));
    sxp=sxp<0?0:sxp>SRC-1?SRC-1:sxp;syp=syp<0?0:syp>SRC-1?SRC-1:syp;
    const o=(syp*SRC+sxp)*4,R=px[o],G=px[o+1],B=px[o+2];
    const v=((R&0xF8)<<8)|((G&0xFC)<<3)|(B>>3),idx=(c*ROWS+r)*2;
    out[idx]=(v>>8)&0xFF;out[idx+1]=v&0xFF;}}
  let bin='';for(let i=0;i<out.length;i++)bin+=String.fromCharCode(out[i]);
  const res=await this.j('/api/photo?slot='+this.curPhotoSlot,{method:'POST',body:btoa(bin)});
  this.st=await this.j('/api/state');this.buildPhotoSlots();
  this.flash(res&&res.ok?'Foto gespeichert':'Fehler');},
 async photoShow(){await this.pick(3,this.curPhotoSlot);this.tab('muster');},
 async photoClear(){await this.j('/api/photo?slot='+this.curPhotoSlot+'&clear=1',{method:'POST'});
  this.st=await this.j('/api/state');this.buildPhotoSlots();this.flash('Geleert');},
 // ---- Betrieb ----
 toggleRun(){this.run(this.st.mode=='DISPLAY'?0:1);},
 async run(on){if(on&&!confirm('Display starten? Das WLAN wird getrennt.'))return;
  await fetch('/api/'+(on?'start':'stop'),{method:'POST'});
  if(on){document.getElementById('pill').textContent='Display laeuft – WLAN getrennt';}},
 async poll(){try{const s=await this.j('/api/status');const p=document.getElementById('pill');
  if(this.st)this.st.mode=s.mode;  // sonst weiss toggleRun() nie, dass schon laeuft
  p.className='pill'+(s.rotating?' on':'')+(s.locked?' lock':'');
  p.textContent=s.calib?'Kalibriere – Stab still halten'
   :s.mode=='DISPLAY'?(Math.round(s.rpm)+' RPM · '+s.effCols+' Sp'+(s.locked?' · LOCK':'')):s.mode;
  document.getElementById('pillStart').textContent=s.mode=='DISPLAY'?'■':'▶';this.renderDev(s);}catch(e){}},
 flash(m){const p=document.getElementById('pill');const o=p.textContent;p.textContent=m;setTimeout(()=>p.textContent=o,1200);},
 // ---- Info-Modal ----
 INFO:{
  bright:['Helligkeit','Gesamthelligkeit der LEDs (1-100). Hoeher = heller, zieht aber mehr Strom. Bei schwachem Akku niedriger halten.'],
  columns:['POV-Spalten','Winkelaufloesung im Vollkreis-, Text- und Mal-Modus (8-192). Mehr = feiner, aber jede Spalte steht kuerzer und wirkt dunkler. Text, Zeichnungen und Fotos heben den Wert automatisch auf ihren eigenen Bedarf an (Text: 6 Spalten pro Zeichen, Foto: 72), damit keine Bildspalte verschluckt wird. Das positionierte Bild ("Bild oben") regelt seine Schaerfe selbst.'],
  blur:['Nachleuchten','Laesst vorherige Frames langsam ausblenden (Bewegungsspur). 0 = gestochen scharf, hoeher = weicher Schweif.'],
  persist:['Winkelbreite','Verbreitert das Bild um zusaetzliche Winkelschritte. 1 = duennste Darstellung.'],
  current:['Stromlimit (mA)','Begrenzt den maximalen LED-Gesamtstrom - schuetzt Akku/Netzteil und haelt Farben/Spannung stabil.'],
  holdus:['Halten (us)','Wie lange eine Spalte maximal stehen bleibt, bevor neu gezeichnet wird.'],
  gain:['Angle Gain','Skalierung der Winkelmessung. 1,000 = Standard (1 Messung = 1 echte Umdrehung). Zu hoch -> Bild erscheint mehrfach. Hiermit trimmst du die Drift weg.'],
  threshold:['Gyro-Schwelle','Ab welcher Drehgeschwindigkeit (rad/s) die Anzeige startet.'],
  iang:['Bildwinkel','Position des stehenden Bildes auf dem Kreis (0-359 Grad). Damit schiebst du es nach oben.'],
  irad:['Bildhoehe','Abstand des Bildzentrums von der Scheibenmitte (0-100%).'],
  iscale:['Bildgroesse','Durchmesser des positionierten Bildes (5-100%).'],
  axis:['Gyro-Achse','Drehachse des Sensors (X/Y/Z). Muss zur Einbaulage passen, sonst wird die Drehung falsch erkannt.'],
  invert:['Richtung invertieren','Dreht die Laufrichtung um - falls Text/Bild spiegelverkehrt erscheint.'],
  plock:['Phase-Lock','Schwerkraft-Drift-Korrektur. Funktioniert nur bei langsamer Drehung (unter ~2,4 U/s); bei schnellem Schleudern saettigt der Sensor - dann aus lassen.'],
  imode:['Bild oben','Zeigt das Muster als kleines, stehendes Bild an einer Stelle statt ueber den ganzen Kreis verteilt.']
 },
 info(k){const d=this.INFO[k];if(!d)return;document.getElementById('modalTitle').textContent=d[0];
  document.getElementById('modalText').textContent=d[1];document.getElementById('modal').classList.add('open');},
 closeInfo(e){document.getElementById('modal').classList.remove('open');},
 // ---- DEV-Tab ----
 kv(host,rows){const h=document.getElementById(host);if(!h)return;
  h.innerHTML=rows.map(r=>`<div class="k">${r[0]}</div><div class="val ${r[2]||''}">${r[1]}</div>`).join('');},
 upt(sec){const m=Math.floor(sec/60),h=Math.floor(m/60);return h>0?h+'h '+(m%60)+'m':m+'m '+(sec%60)+'s';},
 renderDev(s){if(s.rpm===undefined)return;
  this.kv('devRot',[['Dreht',s.rotating?'Ja':'Nein',s.rotating?'good':''],['Lock',s.locked?'Ja':'Nein',s.locked?'good':''],['RPM',Math.round(s.rpm)],['Spalten effektiv',s.effCols],['Ausgabe',s.outHz+' Hz']]);
  this.kv('devSensor',[['Sample-Rate',Math.round(s.sampleHz)+' Hz',s.sampleHz<300?'bad':'good'],['I2C-Fehler',s.fails,s.fails>500?'bad':'good']]);
  this.kv('devSession',[['Lock-Ereignisse',s.locks],['Verworfen',s.rej],['Fehler Ø',s.errAvg+'°'],['Fehler max',s.errMax+'°'],['Drehzahl max',Math.round(s.rpmMax)+' RPM'],['Sample min',Math.round(s.hzMin)+' Hz']]);
  this.kv('devSys',[['Freier Speicher',(s.heap/1024).toFixed(0)+' KB'],['Laufzeit',this.upt(s.uptime)]]);},
};
['mousedown','touchstart'].forEach(ev=>document.getElementById('canvas').addEventListener(ev,e=>{App._d=1;App.paint(e);e.preventDefault();},{passive:false}));
['mousemove','touchmove'].forEach(ev=>document.getElementById('canvas').addEventListener(ev,e=>{if(App._d){App.paint(e);e.preventDefault();}},{passive:false}));
['mouseup','touchend','mouseleave'].forEach(ev=>document.getElementById('canvas').addEventListener(ev,()=>App._d=0));
// Drehen des Telefons aendert die Canvas-Breite -> mit dem gecachten Frame neu zeichnen.
let _rt;addEventListener('resize',()=>{clearTimeout(_rt);_rt=setTimeout(()=>App.draw(App._frame),150);});
App.init();
</script>
</body>
</html>)=====";
