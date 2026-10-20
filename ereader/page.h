// Proxlight Ink web page served by the e-reader (generated from web/index.html by tools/make_page.py)
#pragma once
#include <Arduino.h>
const char PAGE[] PROGMEM = R"HTML(<!DOCTYPE html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><meta name="color-scheme" content="light">
<title>Proxlight Ink</title>
<style>
:root{color-scheme:light}
*{box-sizing:border-box}
body{font-family:-apple-system,Arial,sans-serif;max-width:600px;margin:0 auto;padding:16px;background:#fff;color:#000}
header{display:flex;align-items:center;gap:12px;border-bottom:3px solid #000;padding-bottom:10px;margin-bottom:6px}
.logo{width:42px;height:42px;border-radius:50%;border:3px solid #000;display:grid;place-items:center;font:700 24px Georgia,serif}
h1{font:700 26px Georgia,serif;margin:0;letter-spacing:.3px}
header small{display:block;font-size:12px;color:#444;margin-top:2px}
.box{background:#fff;color:#000;border:2px solid #000;border-radius:12px;padding:14px;margin:12px 0}
#info{font-weight:700;font-size:15px}
#st{min-height:20px;font-size:14px;color:#444;margin:6px 0}
#pv{white-space:pre-wrap;word-break:break-word;font:14px/1.35 ui-monospace,Menlo,monospace;background:#fff;color:#000;border:1px solid #888;padding:10px;margin:8px 0 0;max-height:240px;overflow:auto}
.row{display:flex;gap:8px;margin-top:8px}
button,label.btn{-webkit-appearance:none;appearance:none;flex:1;font-size:18px;padding:14px 8px;border:2px solid #000;border-radius:10px;background:#000;color:#fff;text-align:center;cursor:pointer;display:block;font-family:inherit}
button.big{font-size:22px;padding:20px 8px}
button.alt,label.btn.alt{background:#fff;color:#000}
button:disabled{opacity:.35}
input[type=number]{width:100%;font-size:16px;padding:10px;margin-top:6px;border:2px solid #000;border-radius:8px;background:#fff;color:#000}
progress{width:100%;height:16px;margin-top:10px}
small{color:#444}
#book{font:15px/1.5 Georgia,'Times New Roman',serif;border:1px solid #888;padding:12px 16px;margin-top:10px;max-height:420px;overflow:auto;background:#fff}
#book p{margin:0;text-indent:1.5em;text-align:left}
#book p.n{text-indent:0}
#book h3{font-size:19px;text-align:center;margin:.9em 0 .4em}
#book h3.ch{border-top:2px solid #000;padding-top:.7em;margin-top:1.2em}
#book p.c{text-align:center;text-indent:0}
#book p.q{margin:0 1.8em;font-style:italic;text-indent:0}
#book p.l{padding-left:1.2em;text-indent:0;position:relative}
#book p.l:before{content:"\2022";position:absolute;left:.3em}
#book canvas{display:block;margin:.7em auto;max-width:100%;border:1px solid #ccc;image-rendering:pixelated}
footer{text-align:center;font-size:12px;color:#444;margin:18px 0 6px}
footer b{font-family:Georgia,serif;color:#000}
</style></head><body>
<header><div class="logo">P</div><div><h1>Proxlight Ink</h1><small>E-reader remote &amp; book loader</small></div></header>
<div class="box">
 <div id="info">Connecting...</div>
 <div id="st"></div>
 <div class="row"><button class="rc big alt" onclick="cmd('prev')">&#9664; Prev</button><button class="rc big" onclick="cmd('next')">Next &#9654;</button></div>
 <div class="row"><button class="rc alt" onclick="cmd('prevch')">&#9664;&#9664; Chapter</button><button class="rc alt" onclick="cmd('nextch')">Chapter &#9654;&#9654;</button></div>
 <div class="row"><button class="rc alt" onclick="cmd('smaller')">A&minus; Smaller</button><button class="rc alt" onclick="cmd('bigger')">A+ Bigger</button></div>
 <div class="row"><input id="gp" type="number" min="1" placeholder="Go to page number"><button class="rc" style="flex:0 0 90px" onclick="cmd('goto',$('gp').value||1)">Go</button></div>
 <div class="row"><button class="rc alt" onclick="cmd('first')">Back to start</button><button class="rc alt" onclick="cmd('redraw')">Redraw screen</button></div>
 <pre id="pv" aria-label="Text on the reader's screen right now"></pre>
</div>
<div class="box"><b>Load a book</b><br><small>Choose an .epub (or .txt). Text formatting and pictures are converted here in your browser. Check the preview, then send. The previous book is deleted from the reader first.</small>
 <div class="row" style="align-items:center"><label for="psz" style="flex:1;font-size:15px">Picture size</label>
  <select id="psz" style="flex:1;font-size:16px;padding:10px;border:2px solid #000;border-radius:8px;background:#fff;color:#000"><option value="full">Full screen</option><option value="large" selected>Large (fit page)</option><option value="medium">Medium</option><option value="small">Small</option></select></div>
 <div class="row" style="align-items:center"><label for="pst" style="flex:1;font-size:15px">Picture style</label>
  <select id="pst" style="flex:1;font-size:16px;padding:10px;border:2px solid #000;border-radius:8px;background:#fff;color:#000"><option value="comic" selected>Comic (sharp)</option><option value="photo">Photo (smooth)</option><option value="line">Line art (black &amp; white)</option></select></div>
 <div class="row" style="align-items:center"><label for="prt" style="flex:1;font-size:15px">Turn tall pictures</label>
  <select id="prt" style="flex:1;font-size:16px;padding:10px;border:2px solid #000;border-radius:8px;background:#fff;color:#000"><option value="off" selected>Off</option><option value="cw">Clockwise</option><option value="ccw">Counter-clockwise</option></select></div>
 <small>Changing these re-converts the chosen book. "Turn" rotates portrait pages so they fill the wide screen; then hold the reader sideways.</small>
 <div class="row"><label class="btn alt">Choose EPUB<input id="f" type="file" accept=".epub,.txt" hidden></label></div>
 <p id="msg"><small>Nothing chosen yet.</small></p>
 <progress id="cp" value="0" max="100" style="display:none"></progress>
 <div id="book" style="display:none"></div>
 <div class="row"><button id="up" disabled>Send to e-reader</button></div>
 <div class="row"><button id="clr" class="alt">Clear books &amp; cache</button></div>
 <small>Removes the book on the reader and formats its storage, so every new book gets exactly the same free space.</small>
 <progress id="p" value="0" max="100" style="display:none"></progress>
 <p><small id="free"></small></p>
</div>
<footer><b>Proxlight Ink</b> &middot; Made with ❤️ by Pratyush Mishra</footer>
<script>
const $=id=>document.getElementById(id);
let busy=false,res=null,dev={free:0,sw:800,sh:480,foot:40,margin:14};
function setBusy(b,msg){busy=b;document.querySelectorAll('.rc').forEach(x=>x.disabled=b);$('st').textContent=msg||''}
function parse(t){const i=t.indexOf('\n'),h=t.slice(0,i).split('|');return{ch:h[0],chs:h[1],pg:h[2],pgs:h[3],sz:h[4],szs:h[5],src:+h[6],len:+h[7],bat:h[8]===undefined?-1:+h[8],chg:h[9]==='1',text:t.slice(i+1)}}
function show(s){$('info').textContent='Chapter '+s.ch+'/'+s.chs+'  ·  Page '+s.pg+'/'+s.pgs+'  ·  Size '+s.sz+'/'+s.szs+(s.src===0?'  ·  (demo book)':s.src===2?'  ·  (book error)':'')+(s.bat>=0?'  ·  Battery '+s.bat+'%'+(s.chg?' \u26A1 charging':''):'');$('pv').textContent=s.text.trim()?s.text:'(this page has no text)'}
async function refresh(){try{const r=await fetch('/state');show(parse(await r.text()));return true}catch(e){$('st').textContent='Cannot reach the reader. Are you joined to the Proxlight-Ink Wi-Fi?';return false}}
async function cmd(c,p){if(busy)return;setBusy(true,'Updating the e-ink screen... (a few seconds)');
 try{await fetch('/cmd?c='+c+(p!==undefined?'&p='+encodeURIComponent(p):''));await refresh();setBusy(false,'')}catch(e){setBusy(false,'Connection lost. Try again.')}}
addEventListener('keydown',e=>{if(/INPUT/.test(e.target.tagName))return;
 if(e.key==='ArrowRight'||e.key===' ')cmd('next');else if(e.key==='ArrowLeft')cmd('prev');
 else if(e.key==='+'||e.key==='=')cmd('bigger');else if(e.key==='-')cmd('smaller');else return;e.preventDefault()});
setInterval(()=>{if(!busy&&!sending&&document.visibilityState==='visible')refresh()},30000);refresh();
function loadInfo(){return fetch('/info').then(r=>r.json()).then(j=>{dev=j;$('free').textContent='Space available for a book: '+Math.floor(j.free/1024)+' KB'}).catch(()=>{})}
loadInfo();

/* ---------- text cleaning (the reader's font is plain ASCII) ---------- */
const LIG={'ﬁ':'fi','ﬂ':'fl','ﬀ':'ff','ﬃ':'ffi','ﬄ':'ffl','æ':'ae','Æ':'AE','œ':'oe','Œ':'OE','ß':'ss','ø':'o','Ø':'O','đ':'d','Đ':'D','ł':'l','Ł':'L','þ':'th','©':'(c)','®':'(R)','™':'(TM)','×':'x','÷':'/','½':'1/2','¼':'1/4','¾':'3/4','€':'EUR','£':'GBP','°':' deg','·':'-','•':'-','§':'S.','¶':' ','«':'"','»':'"','‹':"'",'›':"'",'…':'...','—':' - ','–':'-',' ':' ',' ':' ',' ':' ',' ':' ','​':'','­':'','﻿':''};
function clean(t){
 t=t.replace(/[‘’‚‛′]/g,"'").replace(/[“”„‟″]/g,'"').replace(/[‐‑‒―−]/g,'-');
 t=t.replace(/[^\x00-\x7f]/g,c=>c in LIG?LIG[c]:c);
 t=t.normalize('NFKD').replace(/[̀-ͯ]/g,'').replace(/[^\x20-\x7e\s]/g,'');
 return t.replace(/\s+/g,' ').trim()}

/* ---------- zip / epub reading ---------- */
function zipIndex(buf){const dv=new DataView(buf);let e=buf.byteLength-22;
 while(e>=0&&dv.getUint32(e,true)!==0x06054b50)e--;if(e<0)throw Error('Not a valid EPUB file');
 const n=dv.getUint16(e+10,true);let p=dv.getUint32(e+16,true);const f={},td=new TextDecoder();
 for(let i=0;i<n;i++){const nl=dv.getUint16(p+28,true),xl=dv.getUint16(p+30,true),cl=dv.getUint16(p+32,true);
  f[td.decode(new Uint8Array(buf,p+46,nl))]={m:dv.getUint16(p+10,true),c:dv.getUint32(p+20,true),o:dv.getUint32(p+42,true)};p+=46+nl+xl+cl}return f}
async function zipGet(buf,en){const dv=new DataView(buf),o=en.o,s=o+30+dv.getUint16(o+26,true)+dv.getUint16(o+28,true);
 const d=new Uint8Array(buf,s,en.c);if(en.m===0)return d;
 const st=new Blob([d]).stream().pipeThrough(new DecompressionStream('deflate-raw'));return new Uint8Array(await new Response(st).arrayBuffer())}
const txt=u=>new TextDecoder('utf-8').decode(u);
function resolve(base,href){const parts=(base+href.split('#')[0].split('?')[0]).split('/'),o=[];
 for(const x of parts){if(x==='..')o.pop();else if(x!=='.'&&x!=='')o.push(x)}
 let r=o.join('/');try{r=decodeURIComponent(r)}catch(e){}return r}
const dirOf=p=>p.includes('/')?p.replace(/[^/]*$/,''):'';
const MIME={png:'image/png',jpg:'image/jpeg',jpeg:'image/jpeg',gif:'image/gif',webp:'image/webp',svg:'image/svg+xml',bmp:'image/bmp'};

/* ---------- XHTML -> list of paragraphs / pictures ---------- */
const BLOCK=new Set('p div section article aside header footer main ul ol li blockquote h1 h2 h3 h4 h5 h6 pre table thead tbody tfoot tr td th figure figcaption dd dt dl body html address details summary caption'.split(' '));
const SKIP=new Set(['script','style','head','nav','title','noscript','audio','video','math']);
function centered(n){const c=((n.getAttribute('class')||'')+' '+(n.getAttribute('style')||'')).toLowerCase();return /text-align:\s*center|\bcent(er|re)\b|\bchapter-?title\b|\btitle\b/.test(c)}
function walkDoc(doc,docDir,items){
 let buf='',style='',cont=false,first=true;
 const flush=()=>{const t=clean(buf);buf='';if(!t)return;let s=style;if(cont&&s==='')s='\x08';items.push({k:'p',s,t,ch:first});first=false;cont=false};
 const walk=n=>{
  if(n.nodeType===3){buf+=n.nodeValue;return}
  if(n.nodeType!==1)return;
  const tag=n.localName.toLowerCase();
  if(SKIP.has(tag))return;
  if(tag==='img'||tag==='image'){flush();const src=n.getAttribute('src')||n.getAttribute('href')||n.getAttribute('xlink:href')||'';
   if(src&&!/^data:|^https?:/i.test(src)){items.push({k:'i',path:resolve(docDir,src),ch:first});first=false}return}
  if(tag==='br'){flush();cont=true;return}
  if(tag==='hr'){flush();items.push({k:'p',s:'\x05',t:'* * *',ch:first});first=false;return}
  if(BLOCK.has(tag)){flush();const sv=[style,cont];cont=false;
   if(/^h[1-6]$/.test(tag))style='\x03';else if(tag==='blockquote')style='\x06';else if(tag==='li')style='\x07';
   else if(style===''&&(tag==='figcaption'||((tag==='p'||tag==='div')&&centered(n))))style='\x05';
   for(const c of n.childNodes)walk(c);flush();[style,cont]=sv}
  else for(const c of n.childNodes)walk(c)};
 walk(doc.body);flush()}

/* ---------- picture -> dithered 1-bit bitmap ---------- */
function loadImg(blob){return new Promise((ok,no)=>{const u=URL.createObjectURL(blob),i=new Image();
 i.onload=()=>{ok(i);setTimeout(()=>URL.revokeObjectURL(u),3000)};i.onerror=()=>{URL.revokeObjectURL(u);no(Error('bad image'))};i.src=u})}
function resample(src,sw,sh,dw,dh){let cur=src,cw=sw,ch=sh;           // halve step by step: much sharper than one big jump
 while(cw>dw*2||ch>dh*2){const nw=Math.max(dw,Math.ceil(cw/2)),nh=Math.max(dh,Math.ceil(ch/2));
  const c=document.createElement('canvas');c.width=nw;c.height=nh;const x=c.getContext('2d');x.imageSmoothingQuality='high';x.fillStyle='#fff';x.fillRect(0,0,nw,nh);x.drawImage(cur,0,0,nw,nh);cur=c;cw=nw;ch=nh}
 return cur}
function sharpen(g,w,h,amt){const b=new Float32Array(w*h);                // unsharp mask (3x3 blur)
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){let a=0,n=0;for(let dy=-1;dy<=1;dy++){const yy=y+dy;if(yy<0||yy>=h)continue;for(let dx=-1;dx<=1;dx++){const xx=x+dx;if(xx<0||xx>=w)continue;a+=g[yy*w+xx];n++}}b[y*w+x]=a/n}
 for(let i=0;i<w*h;i++)g[i]=Math.max(0,Math.min(255,g[i]+amt*(g[i]-b[i])))}
function otsu(g){const hist=new Uint32Array(256);for(let i=0;i<g.length;i++)hist[g[i]|0]++;
 let sum=0;for(let i=0;i<256;i++)sum+=i*hist[i];let sb=0,wb=0,best=0,th=128;
 for(let t=0;t<256;t++){wb+=hist[t];if(!wb)continue;const wf=g.length-wb;if(!wf)break;sb+=t*hist[t];
  const mb=sb/wb,mf=(sum-sb)/wf,v=wb*wf*(mb-mf)*(mb-mf);if(v>best){best=v;th=t}}return th}
async function toBitmap(blob,maxW,maxH,o){
 o=o||{st:'photo',rot:'off'};
 const img=await loadImg(blob);const iw=img.naturalWidth||img.width||maxW,ih=img.naturalHeight||img.height||maxH;
 if(iw<30&&ih<30)return null;                                  // tiny decoration: skip
 const turn=o.rot!=='off'&&ih>iw*1.15,lw=turn?ih:iw,lh=turn?iw:ih;
 const s=Math.min(maxW/lw,maxH/lh,2),w=Math.max(1,Math.round(lw*s)),h=Math.max(1,Math.round(lh*s));
 const dw=Math.max(1,Math.round(iw*s)),dh=Math.max(1,Math.round(ih*s));
 const small=resample(img,iw,ih,dw,dh);
 const cv=document.createElement('canvas');cv.width=w;cv.height=h;const cx=cv.getContext('2d',{willReadFrequently:true});
 cx.fillStyle='#fff';cx.fillRect(0,0,w,h);cx.imageSmoothingQuality='high';
 if(turn){cx.translate(w/2,h/2);cx.rotate(o.rot==='cw'?Math.PI/2:-Math.PI/2);cx.drawImage(small,-dw/2,-dh/2,dw,dh)}
 else cx.drawImage(small,0,0,w,h);
 const d=cx.getImageData(0,0,w,h).data,g=new Float32Array(w*h),hist=new Uint32Array(256);
 for(let i=0;i<w*h;i++){const v=Math.round(.299*d[i*4]+.587*d[i*4+1]+.114*d[i*4+2]);g[i]=v;hist[v]++}
 let lo=0,hi=255,acc=0,t=w*h*.01;                             // auto-contrast (1%..99%)
 for(let v=0;v<256;v++){acc+=hist[v];if(acc>t){lo=v;break}}acc=0;
 for(let v=255;v>=0;v--){acc+=hist[v];if(acc>t){hi=v;break}}
 if(hi-lo<40){lo=0;hi=255}
 const sc=255/(hi-lo);for(let i=0;i<w*h;i++)g[i]=Math.max(0,Math.min(255,(g[i]-lo)*sc));
 if(o.st!=='photo')sharpen(g,w,h,o.st==='line'?1.2:0.9);
 const stride=(w+7)>>3,out=new Uint8Array(4+stride*h);
 out[0]=w&255;out[1]=w>>8;out[2]=h&255;out[3]=h>>8;
 const put=(x,y)=>{out[4+y*stride+(x>>3)]|=0x80>>(x&7)};
 if(o.st==='line'){const th=otsu(g);for(let y=0;y<h;y++)for(let x=0;x<w;x++)if(g[y*w+x]<th)put(x,y)}
 else if(o.st==='comic'){                                       // Atkinson: crisper, higher contrast than Floyd-Steinberg
  for(let y=0;y<h;y++)for(let x=0;x<w;x++){const i=y*w+x,old=g[i],nv=old<128?0:255,e=(old-nv)/8;g[i]=nv;
   if(x+1<w)g[i+1]+=e;if(x+2<w)g[i+2]+=e;
   if(y+1<h){if(x>0)g[i+w-1]+=e;g[i+w]+=e;if(x+1<w)g[i+w+1]+=e}
   if(y+2<h)g[i+2*w]+=e;
   if(nv===0)put(x,y)}}
 else{for(let y=0;y<h;y++){const r=y*w;
  for(let x=0;x<w;x++){const o2=g[r+x],nv=o2<128?0:255,e=o2-nv;g[r+x]=nv;       // Floyd-Steinberg
   if(x+1<w)g[r+x+1]+=e*7/16;
   if(y+1<h){if(x>0)g[r+w+x-1]+=e*3/16;g[r+w+x]+=e*5/16;if(x+1<w)g[r+w+x+1]+=e/16}
   if(nv===0)put(x,y)}}}
 return{w,h,bytes:out}}

/* ---------- whole book ---------- */
async function convert(file,prog){
 const items=[];let cover=null;let z=null,buf=null;
 if(/\.txt$/i.test(file.name)){
  const ls=(await file.text()).split(/\r?\n/).map(clean).filter(Boolean);if(!ls.length)throw Error('Empty file');
  ls.forEach((t,i)=>items.push({k:'p',s:'',t,ch:i===0}))}
 else{
  buf=await file.arrayBuffer();z=zipIndex(buf);
  const get=async p=>{const e=z[p];if(!e)throw Error('Missing '+p);return txt(await zipGet(buf,e))};
  const xml=s=>new DOMParser().parseFromString(s,'application/xml');
  const opfPath=xml(await get('META-INF/container.xml')).querySelector('rootfile').getAttribute('full-path');
  const base=dirOf(opfPath),opf=xml(await get(opfPath));
  const man={};opf.querySelectorAll('manifest > item').forEach(i=>man[i.getAttribute('id')]={href:i.getAttribute('href'),prop:i.getAttribute('properties')||''});
  const cm=opf.querySelector('metadata meta[name="cover"]');
  const ci=[...Object.values(man)].find(m=>/cover-image/.test(m.prop))||(cm&&man[cm.getAttribute('content')]);
  if(ci&&ci.href)cover=resolve(base,ci.href);
  const refs=[...opf.querySelectorAll('spine > itemref')];let n=0;
  for(const ir of refs){prog(++n/refs.length*45);
   const m=man[ir.getAttribute('idref')];if(!m)continue;const path=resolve(base,m.href);if(!z[path])continue;
   try{const doc=new DOMParser().parseFromString(await get(path),'text/html'),mark=items.length;
    walkDoc(doc,dirOf(path),items);
    const mine=items.slice(mark),tl=mine.filter(x=>x.k==='p').reduce((a,x)=>a+x.t.length,0);
    if(tl<40&&!mine.some(x=>x.k==='i'))items.length=mark}catch(e){}}
  if(cover&&z[cover]&&!items.some(x=>x.k==='i'&&x.path===cover))items.unshift({k:'i',path:cover,ch:true})}
 if(!items.some(x=>x.k==='p'||x.k==='i'))throw Error('No readable text or pictures found in this book');
 // budget: reader storage minus the text itself
 const textBytes=items.reduce((a,x)=>a+(x.k==='p'?x.t.length+2:8),0);
 let budget=dev.free-textBytes-24*1024;
 const szm=$('psz').value,full=szm==='full',k={full:1,large:1,medium:.8,small:.6}[szm]||1;
 const maxW=Math.floor((full?dev.sw:dev.sw-2*dev.margin)*k),maxH=Math.floor((full?dev.sh:dev.sh-dev.foot-2*dev.margin)*k);
 const popt={st:$('pst').value,rot:$('prt').value};
 const images=[],idx={};let dropped=0,total=items.filter(x=>x.k==='i').length,done=0;
 for(const it of items){if(it.k!=='i')continue;prog(45+(++done/Math.max(1,total))*50);
  if(it.path in idx){it.idx=idx[it.path];continue}
  let bm=null;
  try{const e=z&&z[it.path];if(e){const ext=(it.path.split('.').pop()||'').toLowerCase();
   bm=await toBitmap(new Blob([await zipGet(buf,e)],{type:MIME[ext]||'image/png'}),maxW,maxH,popt)}}catch(e){bm=null}
  if(!bm){idx[it.path]=null;it.idx=null;continue}
  if(bm.bytes.length>budget){dropped++;idx[it.path]=null;it.idx=null;continue}
  budget-=bm.bytes.length;idx[it.path]=images.length;it.idx=images.length;images.push(bm)}
 let out='',pending=false,chs=0,blocks=[];
 for(const it of items){if(it.ch)pending=true;
  if(it.k==='p'){out+=(pending?'\x01':'')+it.s+it.t+'\n';if(pending)chs++;pending=false;blocks.push(it)}
  else if(it.idx!=null){out+=(pending?'\x01':'')+'\x02'+it.idx+'\n';if(pending)chs++;pending=false;blocks.push(it)}}
 prog(100);
 return{text:out,images,chapters:chs,dropped,blocks,bytes:out.length+images.reduce((a,b)=>a+b.bytes.length,0)}}

/* ---------- preview (what the reader will show, formatted) ---------- */
function bitmapCanvas(bm){const c=document.createElement('canvas');c.width=bm.w;c.height=bm.h;const x=c.getContext('2d'),id=x.createImageData(bm.w,bm.h),st=(bm.w+7)>>3;
 for(let y=0;y<bm.h;y++)for(let i=0;i<bm.w;i++){const b=(bm.bytes[4+y*st+(i>>3)]>>(7-(i&7)))&1,o=(y*bm.w+i)*4,v=b?0:255;id.data[o]=id.data[o+1]=id.data[o+2]=v;id.data[o+3]=255}
 x.putImageData(id,0,0);return c}
function renderPreview(r){const box=$('book');box.innerHTML='';let n=0;
 for(const b of r.blocks){if(++n>150){const p=document.createElement('p');p.className='c';p.textContent='... preview shows the beginning only ...';box.appendChild(p);break}
  if(b.k==='i'){box.appendChild(bitmapCanvas(r.images[b.idx]));continue}
  const s=b.s;let e;
  if(s==='\x03'){e=document.createElement('h3');if(b.ch)e.className='ch'}
  else{e=document.createElement('p');e.className=s==='\x05'?'c':s==='\x06'?'q':s==='\x07'?'l':s==='\x08'?'n':''}
  e.textContent=b.t;box.appendChild(e)}
 box.style.display='block'}

/* ---------- clear ---------- */
$('clr').onclick=async()=>{if(sending||busy)return;if(!confirm('Delete the book on the reader and clear its storage?'))return;
 sending=true;$('clr').disabled=true;$('up').disabled=true;res=null;$('f').value='';$('book').style.display='none';$('msg').innerHTML='<b>Clearing the reader... (the screen will show the demo book)</b>';
 try{await post('/clear');$('msg').innerHTML='<b>Cleared. The reader storage is empty again.</b>';await new Promise(r=>setTimeout(r,6000));await loadInfo();await refresh()}
 catch(e){$('msg').innerHTML='<b>Could not clear: '+e.message+'</b>'}
 sending=false;$('clr').disabled=false};

/* ---------- choose + send ---------- */
let sending=false;
let curFile=null;
async function load(file){$('up').disabled=true;res=null;$('book').style.display='none';
 await loadInfo();
 $('msg').innerHTML='<small>Converting text and pictures... please wait.</small>';$('cp').style.display='block';$('cp').value=0;
 try{res=await convert(file,v=>{$('cp').value=v});
  const kb=Math.ceil(res.bytes/1024);
  $('msg').innerHTML='<b>'+file.name.replace(/</g,'')+'</b><br><small>'+res.chapters+' chapters &middot; '+res.images.length+' pictures &middot; '+kb+' KB on the reader'+
   (res.dropped?'<br><b>'+res.dropped+' picture(s) skipped: not enough space on the reader.</b>':'')+'</small>';
  if(res.text.length>dev.free-8192){$('msg').innerHTML+='<br><b>Too big for the reader ('+kb+' KB, only '+Math.floor(dev.free/1024)+' KB free).</b>';res=null;return}
  renderPreview(res);$('up').disabled=false}
 catch(err){res=null;$('msg').innerHTML='<b>Error: '+err.message+'</b>'}
 finally{$('cp').style.display='none'}}
$('f').onchange=e=>{const file=e.target.files[0];if(!file)return;e.target.value='';curFile=file;load(file)};
for(const id of ['psz','pst','prt']){
 try{const v=localStorage.getItem('pk_'+id);if(v)$(id).value=v}catch(e){}
 $(id).onchange=()=>{try{localStorage.setItem('pk_'+id,$(id).value)}catch(e){}if(curFile&&!sending)load(curFile)}}
function post(url,blob,name,onp){return new Promise((ok,no)=>{const x=new XMLHttpRequest();x.open('POST',url);
 if(name&&onp)x.upload.onprogress=e=>{if(e.lengthComputable)onp(e.loaded)};
 x.onload=()=>x.status===200?ok():no(Error(x.responseText||('HTTP '+x.status)));x.onerror=()=>no(Error('Connection lost'));
 if(!blob){x.send();return}const fd=new FormData();fd.append('f',blob,name);x.send(fd)})}
$('up').onclick=async()=>{if(!res)return;sending=true;$('up').disabled=true;const P=$('p');P.style.display='block';P.value=0;
 const files=res.images.map((b,i)=>({n:'i'+i+'.bin',b:new Blob([b.bytes])}));files.push({n:'book.txt',b:new Blob([res.text],{type:'text/plain'})});
 const tot=files.reduce((a,f)=>a+f.b.size,0);let sent=0;
 try{$('msg').innerHTML='<b>Clearing the previous book on the reader...</b>';await post('/begin');
  for(let i=0;i<files.length;i++){const f=files[i];
   $('msg').innerHTML='<b>Sending '+(i+1)+' / '+files.length+'...</b>';
   await post('/upload',f.b,f.n,l=>{P.value=(sent+l)/tot*100});sent+=f.b.size;P.value=sent/tot*100}
  await post('/done');
  $('msg').innerHTML='<b>Sent. The reader is formatting your book... (e-ink needs a few seconds)</b>';
  let ok=false;
  for(let i=0;i<25&&!ok;i++){await new Promise(r=>setTimeout(r,3000));
   try{const s=parse(await (await fetch('/state')).text());if(s.src===1&&s.len>0){ok=true;show(s);
    $('msg').innerHTML='<b>Your book is on the reader: '+s.chs+' chapters, '+s.pgs+' pages.</b>'}
    else if(s.src===2){$('msg').innerHTML='<b>The reader could not open the book. Please send it again.</b>';break}}catch(e){}}
  if(!ok&&!/could not/.test($('msg').innerText))$('msg').innerHTML='<b>Sent, but the reader still shows the demo book. Wait 20 seconds, press Redraw screen, or send again.</b>';
  sending=false;loadInfo()}
 catch(err){sending=false;$('msg').innerHTML='<b>Upload failed: '+err.message+'</b>';$('up').disabled=false}};
</script></body></html>
)HTML";
