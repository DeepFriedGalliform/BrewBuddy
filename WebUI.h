// WebUI.h - the Brew Buddy web page (served from flash by the Wi-Fi hotspot).
// Keep this file in the same folder as BrewBuddy.ino.
#pragma once
#include <pgmspace.h>

static const char WEB_UI[] PROGMEM = R"BBHTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Brew Buddy</title>
<style>
:root{--bg:#14100d;--card:#221b16;--ink:#f3e5d0;--dim:#a89a88;--gold:#e2b857;--red:#e53935;--line:#3a2f27}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.4 system-ui,-apple-system,sans-serif}
header{padding:14px 16px;display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid var(--line)}
h1{font-size:18px;margin:0;color:var(--gold)} h3{margin:0 0 4px;color:var(--gold)}
#st{font-size:13px;color:var(--dim);text-align:right}
nav{display:flex;gap:6px;padding:10px 12px;position:sticky;top:0;background:var(--bg);z-index:5}
nav button{flex:1}
button,select,input{font:inherit;color:var(--ink);background:#2c231c;border:1px solid var(--line);border-radius:8px;padding:8px 10px}
button{cursor:pointer}
button.pri,button.on{background:var(--gold);color:#201509;border-color:var(--gold);font-weight:600}
button.bad{background:var(--red);border-color:var(--red)}
main{max-width:720px;margin:0 auto;padding:0 12px 90px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px;margin:10px 0}
.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}.sp{flex:1}
.badge{font-size:12px;background:#3a2f27;border-radius:10px;padding:2px 8px;color:var(--gold)}
.sub{color:var(--dim);font-size:13px;margin:4px 0 8px}
label{display:block;font-size:13px;color:var(--dim);margin:10px 0 4px}
input,select{width:100%}
.g2{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.step{display:flex;flex-wrap:wrap;gap:6px;align-items:center;margin:8px 0;padding:8px;border:1px solid var(--line);border-radius:10px}
.step select{flex:2 1 130px;width:auto}.ig{display:flex;align-items:center;gap:4px;flex:1 1 84px}
.ig b{font-weight:400;color:var(--dim);font-size:13px}
.step button{padding:6px 9px}
#bar{position:fixed;left:0;right:0;bottom:0;padding:10px 12px;background:#1d1611;border-top:1px solid var(--line);display:none;gap:8px;justify-content:center;align-items:center;z-index:6}
#modal{position:fixed;inset:0;background:#000b;display:none;align-items:flex-start;justify-content:center;overflow:auto;padding:14px;z-index:10}
#modal>div{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px;width:100%;max-width:560px}
input[type=range]{padding:0}
</style></head><body>
<header><h1>&#9749; Brew Buddy</h1><div id="st"></div></header>
<nav><button class="on" id="t_rec" onclick="tab('rec')">Recipes</button><button id="t_set" onclick="tab('set')">Settings</button><button id="t_hist" onclick="tab('hist')">History</button><button id="t_bak" onclick="tab('bak')">Backup</button></nav>
<main>
<section id="rec">
 <div class="row"><select id="msel" onchange="chM()"></select><button class="pri" onclick="add()">+ New recipe</button></div>
 <div class="sub" id="rsub"></div><div id="list"></div>
</section>
<section id="set" style="display:none"><div class="card"><div id="setf"></div><p><button class="pri" onclick="saveSet()">Save settings</button></p></div></section>
<section id="hist" style="display:none">
 <div class="row"><button onclick="loadHist()">Refresh</button><span class="sp"></span><button onclick="csv()">Download CSV</button><button class="bad" onclick="clr()">Clear</button></div>
 <div class="sub" id="hsub"></div><div id="hl"></div>
</section>
<section id="bak" style="display:none">
 <div class="card"><h3>Backup</h3><div class="sub">Download every recipe and setting as a file.</div><button onclick="dl()">Download backup</button></div>
 <div class="card"><h3>Restore from backup</h3><div class="sub">Replaces the recipes and settings on the Brew Buddy.</div><input type="file" accept=".json,application/json" onchange="upl(event)"></div>
 <div class="card"><h3>Restore default recipes</h3><div class="sub">Throws away all recipe changes and reloads the built-in recipe table.</div><button class="bad" onclick="rest()">Restore defaults</button></div>
</section>
</main>
<div id="bar"><span id="bt">Unsaved changes</span><button onclick="discard()">Discard</button><button class="pri" onclick="saveList()">Save to Brew Buddy</button></div>
<div id="modal"><div id="ed"></div></div>
<script>
const $=id=>document.getElementById(id);
let S,M=1,L=[],dirty=false,E,EI=-1,tt,H=[],synced=false;
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
function toast(t,bad){const e=$('st');e.textContent=t;e.style.color=bad?'#ff8a80':'#9ccc65';clearTimeout(tt);tt=setTimeout(()=>e.textContent='',4000)}
async function api(p,o){const r=await fetch(p,o);if(!r.ok)throw new Error((await r.text())||r.status);return r}
const post=(p,b)=>api(p,{method:'POST',headers:{'Content-Type':'text/plain'},body:b});
const isW=t=>S.types[t]&&S.types[t][1]==1;
const wat=r=>r.s.reduce((a,s)=>a+(isW(s[0])?s[3]:0),0);
const tim=r=>r.s.reduce((a,s)=>a+s[2],0);
const mmss=x=>String(Math.floor(x/60)).padStart(2,'0')+':'+String(x%60).padStart(2,'0');
const tdisp=t=>S.cfg.unit?Math.round(t*.18+32):Math.round(t/10);
const tset=v=>S.cfg.unit?Math.round((v-32)*50/9):Math.round(v*10);
const tu=()=>S.cfg.unit?'\u00b0F':'\u00b0C';
const GN=['Fine','Med-Fine','Med','Med-Coarse','Coarse','Very Coarse'];
function gdesc(g){const a=S.cfg.gmin,b=S.cfg.gmax;if(b<=a)return'';return GN[Math.max(0,Math.min(5,Math.floor(((g-a)*5+(b-a)/2)/(b-a))))]}
const gtxt=g=>(g/10).toFixed(1)+(S.cfg.gmode?' ('+gdesc(g)+')':'');
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
function ser(list){return list.map(r=>'R|'+String(r.n).replace(/[|\r\n]/g,' ').slice(0,S.max.name)+'|'+[r.c,r.cf,r.g,r.t].join('|')+'\n'+r.s.map(s=>'S|'+s.join('|')).join('\n')).join('\n')}
function cfgText(c){return 'slots='+c.slots.join(',')+'\nready='+c.ready+'\nvib='+c.vib+'\nsleep='+c.sleep+'\ngmode='+c.gmode+'\nunit='+c.unit+'\ngmin='+c.gmin+'\ngmax='+c.gmax+'\nbright='+(c.bright||100)}
async function load(){
 try{S=await(await api('/api/state')).json()}catch(e){toast('Cannot reach Brew Buddy: '+e.message,1);return}
 const ms=$('msel');ms.innerHTML='';S.methods.forEach((n,i)=>{if(i>0){const o=document.createElement('option');o.value=i;o.textContent=n;ms.appendChild(o)}});
 if(M<1||M>=S.methods.length)M=1;ms.value=M;loadList();fillSet();
 if(!synced){synced=true;post('/api/time',String(Math.floor(Date.now()/1000))).catch(()=>{})}
}
function loadList(){L=JSON.parse(JSON.stringify(S.recipes[M]));setDirty(false)}
function chM(){const n=+$('msel').value;if(dirty&&!confirm('Discard unsaved changes to '+S.methods[M]+'?')){$('msel').value=M;return}M=n;loadList()}
function setDirty(d){dirty=d!==false;$('bar').style.display=dirty?'flex':'none';render()}
function discard(){loadList()}
function render(){
 const box=$('list');box.innerHTML='';$('rsub').textContent=L.length+' of '+S.max.recipes+' recipes';
 L.forEach((r,i)=>{const w=wat(r),c=document.createElement('div');c.className='card';
  c.innerHTML='<div class="row"><b>'+esc(r.n)+'</b><span class="badge">'+r.c+'-cup</span></div>'+
  '<div class="sub">'+(r.cf/10).toFixed(1)+' g coffee \u00b7 '+w+' g water \u00b7 1:'+(w*10/r.cf).toFixed(1)+' \u00b7 grind '+gtxt(r.g)+' \u00b7 '+tdisp(r.t)+tu()+' \u00b7 '+mmss(tim(r))+' \u00b7 '+r.s.length+' steps</div>'+
  '<div class="row"><button onclick="edit('+i+')">Edit</button><button onclick="cp('+i+')">Copy</button><span class="sp"></span><button onclick="mv('+i+',-1)">&uarr;</button><button onclick="mv('+i+',1)">&darr;</button><button class="bad" onclick="del('+i+')">Delete</button></div>';
  box.appendChild(c)});
}
function mv(i,d){const j=i+d;if(j<0||j>=L.length)return;[L[i],L[j]]=[L[j],L[i]];setDirty()}
function del(i){if(L.length<2){toast('Keep at least one recipe',1);return}if(confirm('Delete "'+L[i].n+'"?')){L.splice(i,1);setDirty()}}
function cp(i){if(L.length>=S.max.recipes){toast('Recipe limit reached',1);return}const r=JSON.parse(JSON.stringify(L[i]));r.n=(r.n+' copy').slice(0,S.max.name);L.splice(i+1,0,r);setDirty()}
function add(){if(L.length>=S.max.recipes){toast('Recipe limit reached',1);return}
 const b=L[0]?JSON.parse(JSON.stringify(L[0])):{c:1,cf:150,g:60,t:940,s:[[0,0,15,240],[3,0,120,0]]};b.n='New recipe';L.push(b);setDirty();edit(L.length-1)}
async function saveList(){try{await post('/api/recipes?m='+M,ser(L));toast('Saved to Brew Buddy');await load()}catch(e){toast('Save failed: '+e.message,1)}}
// ---- recipe editor
function edit(i){EI=i;E=JSON.parse(JSON.stringify(L[i]));drawEd();$('modal').style.display='flex'}
function closeEd(){$('modal').style.display='none'}
function drawEd(){
 $('ed').innerHTML='<h3>Edit recipe</h3><label>Name</label><input id="en" maxlength="'+S.max.name+'" value="'+esc(E.n)+'" oninput="upd()">'+
 '<div class="g2"><div><label>Size</label><select id="ec" onchange="upd()"><option value="1">1-cup</option><option value="2">2-cup</option></select></div>'+
 '<div><label>Coffee (g)</label><input id="ef" type="number" step="0.1" min="1" max="60" value="'+E.cf/10+'" oninput="upd()"></div>'+
 '<div><label>Grind <span id="gd"></span></label><input id="eg" type="number" step="0.1" min="'+S.cfg.gmin/10+'" max="'+S.cfg.gmax/10+'" value="'+E.g/10+'" oninput="upd()"></div>'+
 '<div><label>Temperature ('+tu()+')</label><input id="et" type="number" step="1" value="'+tdisp(E.t)+'" oninput="upd()"></div></div>'+
 '<div class="sub" id="esum"></div><b>Steps</b><div id="esteps"></div>'+
 '<div class="row"><button onclick="addStep()">+ Add step</button><span class="sp"></span><button onclick="closeEd()">Cancel</button><button class="pri" onclick="commitEd()">Done</button></div>';
 $('ec').value=E.c;steps();upd();
}
function upd(){
 E.n=$('en').value;E.c=+$('ec').value;E.cf=Math.round(parseFloat($('ef').value||0)*10);E.g=Math.round(parseFloat($('eg').value||0)*10);E.t=tset(parseFloat($('et').value||0));
 $('gd').textContent=S.cfg.gmode?'('+gdesc(E.g)+')':'';
 const w=wat(E);$('esum').textContent=w+' g water \u00b7 ratio 1:'+(E.cf?(w*10/E.cf).toFixed(1):'-')+' \u00b7 total '+mmss(tim(E));
}
function steps(){
 const al=S.types.map((t,i)=>i).filter(i=>(S.mask[M]>>i)&1),hv=S.valve[M]==1;
 $('esteps').innerHTML=E.s.map((s,i)=>{
  const ids=al.includes(s[0])?al:al.concat([s[0]]);
  return '<div class="step"><select onchange="sv('+i+',0,this.value,1)">'+ids.map(t=>'<option value="'+t+'"'+(t==s[0]?' selected':'')+'>'+esc(S.types[t][0])+'</option>').join('')+'</select>'+
  (hv?'<select style="flex:1 1 90px;width:auto" onchange="sv('+i+',1,this.value)"><option value="0"'+(s[1]?'':' selected')+'>Valve closed</option><option value="1"'+(s[1]?' selected':'')+'>Valve open</option></select>':'')+
  '<div class="ig"><input type="number" min="1" max="900" value="'+s[2]+'" oninput="sv('+i+',2,this.value)"><b>s</b></div>'+
  (isW(s[0])?'<div class="ig"><input type="number" min="0" max="1000" value="'+s[3]+'" oninput="sv('+i+',3,this.value)"><b>g</b></div>':'')+
  '<button onclick="ms('+i+',-1)">&uarr;</button><button onclick="ms('+i+',1)">&darr;</button><button class="bad" onclick="rs('+i+')">&times;</button></div>'}).join('');
}
function sv(i,k,v,re){v=parseInt(v)||0;E.s[i][k]=v;if(k==0){if(isW(v)&&!E.s[i][3])E.s[i][3]=50;if(!isW(v))E.s[i][3]=0}if(re)steps();upd()}
function ms(i,d){const j=i+d;if(j<0||j>=E.s.length)return;[E.s[i],E.s[j]]=[E.s[j],E.s[i]];steps()}
function rs(i){if(E.s.length<2){toast('A recipe needs at least one step',1);return}E.s.splice(i,1);steps();upd()}
function addStep(){if(E.s.length>=S.max.steps){toast('Step limit reached',1);return}const al=S.types.map((t,i)=>i).filter(i=>(S.mask[M]>>i)&1);
 const last=E.s[E.s.length-1];E.s.push([al.includes(3)?3:al[0],last?last[1]:0,30,0]);steps();upd()}
function commitEd(){
 upd();E.n=E.n.replace(/[|\r\n]/g,' ').trim();
 if(!E.n){toast('Give the recipe a name',1);return}
 if(E.cf<10||E.cf>600){toast('Coffee must be 1 - 60 g',1);return}
 E.g=clamp(E.g,S.cfg.gmin,S.cfg.gmax);E.t=clamp(E.t,400,1000);
 E.s.forEach(s=>{s[2]=clamp(s[2],1,900);if(!isW(s[0]))s[3]=0;s[3]=clamp(s[3],0,1000)});
 L[EI]=E;closeEd();setDirty();
}
// ---- settings
function mopts(sel){return S.methods.map((n,i)=>'<option value="'+i+'"'+(i==sel?' selected':'')+'>'+esc(n)+'</option>').join('')}
function fillSet(){const c=S.cfg,sl=[0,1,2,3].map(i=>'<label>Brew '+(i+1)+' ('+(i*90)+'\u00b0 rotation)</label><select id="sl'+i+'">'+mopts(c.slots[i])+'</select>').join('');
 const so=[0,1,2,5,10,15,30].map(v=>'<option value="'+v+'"'+(v==c.sleep?' selected':'')+'>'+(v?v+' min':'Never')+'</option>').join('');
 $('setf').innerHTML=sl+'<label>Get ready time (seconds, 0 = skip)</label><input id="sr" type="number" min="0" max="30" value="'+c.ready+'">'+
 '<label>Vibration sensitivity (espresso auto): <span id="svv">'+c.vib+'</span>%</label><input id="sv" type="range" min="0" max="100" value="'+c.vib+'" oninput="$(\'svv\').textContent=this.value">'+
 '<label>Screen brightness: <span id="sbv">'+(c.bright||100)+'</span>%</label><input id="sb" type="range" min="10" max="100" value="'+(c.bright||100)+'" oninput="$(\'sbv\').textContent=this.value">'+
 '<label>Sleep after</label><select id="ss">'+so+'</select>'+
 '<label>Grind display</label><select id="sg"><option value="0"'+(c.gmode?'':' selected')+'>Number</option><option value="1"'+(c.gmode?' selected':'')+'>Description</option></select>'+
 '<div class="g2"><div><label>Min grind</label><input id="sgn" type="number" step="0.1" min="0.1" max="48" value="'+c.gmin/10+'"></div><div><label>Max grind</label><input id="sgx" type="number" step="0.1" min="1.1" max="50" value="'+c.gmax/10+'"></div></div>'+
 '<label>Temperature unit</label><select id="su"><option value="0"'+(c.unit?'':' selected')+'>Celsius</option><option value="1"'+(c.unit?' selected':'')+'>Fahrenheit</option></select>'}
async function saveSet(){
 const c={slots:[0,1,2,3].map(i=>+$('sl'+i).value),ready:clamp(parseInt($('sr').value)||0,0,30),vib:+$('sv').value,bright:+$('sb').value,sleep:+$('ss').value,gmode:+$('sg').value,unit:+$('su').value,
  gmin:Math.round(parseFloat($('sgn').value)*10),gmax:Math.round(parseFloat($('sgx').value)*10)};
 if(!(c.gmin>=1)||!(c.gmax>=c.gmin+10)||c.gmax>500){toast('Grind range must span at least 1.0 and max out at 50',1);return}
 try{await post('/api/config',cfgText(c));toast('Settings saved');await load()}catch(e){toast('Save failed: '+e.message,1)}}
// ---- history
const KT=['Brew','Espresso (manual)','Espresso (auto)'];
const when=t=>t>1700000000?new Date(t*1000).toLocaleString():'\u2014';
const dur=(d,k)=>k?d.toFixed(1)+' s':mmss(Math.round(d));
async function loadHist(){
 try{H=(await(await api('/api/history')).json()).h}catch(e){toast('Cannot read history: '+e.message,1);return}
 $('hsub').textContent=H.length?H.length+' most recent entries (the Brew Buddy keeps the last 20). Dates need the clock set by this page, so entries made before the last power-off may show a dash.':'No brews logged yet.';
 $('hl').innerHTML=H.map(h=>'<div class="card"><div class="row"><b>'+esc(h.n)+'</b><span class="badge">'+esc(h.m)+'</span><span class="badge">'+h.c+'-cup</span><span class="sp"></span><b>'+dur(h.d,h.k)+'</b></div><div class="sub">'+when(h.t)+' \u00b7 '+KT[h.k]+(h.l.length?' \u00b7 laps: '+h.l.map(x=>x.toFixed(1)).join(', ')+' s':'')+'</div></div>').join('');
}
function csv(){
 if(!H.length){toast('Nothing to export',1);return}
 const q=v=>'"'+String(v).replace(/"/g,'""')+'"';
 const rows=[['Date','Method','Recipe','Cups','Type','Time (s)','Laps (s)'].map(q).join(',')].concat(H.map(h=>[when(h.t),h.m,h.n,h.c,KT[h.k],h.d.toFixed(1),h.l.join(' ')].map(q).join(',')));
 const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([rows.join('\n')],{type:'text/csv'}));a.download='brewbuddy-history.csv';a.click();
}
async function clr(){if(!confirm('Clear the brew history?'))return;try{await post('/api/history/clear','');toast('History cleared');loadHist()}catch(e){toast('Failed: '+e.message,1)}}
// ---- backup
function dl(){const o={brewbuddy:1,methods:S.methods,cfg:S.cfg,recipes:S.recipes};const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([JSON.stringify(o)],{type:'application/json'}));a.download='brewbuddy-backup.json';a.click()}
function upl(ev){const f=ev.target.files[0];if(!f)return;const rd=new FileReader();
 rd.onload=async()=>{try{const o=JSON.parse(rd.result);if(!o.brewbuddy||!o.recipes||!o.cfg||!o.methods)throw new Error('Not a Brew Buddy backup');
  if(!confirm('Replace all recipes and settings on the Brew Buddy with this backup?'))return;
  const mp=i=>S.methods.indexOf(o.methods[i]),c=Object.assign({},o.cfg);c.slots=o.cfg.slots.map(i=>{const k=mp(i);return k<0?1:k});
  await post('/api/config',cfgText(c));
  for(let i=1;i<o.methods.length;i++){const m=mp(i);if(m>0&&o.recipes[i]&&o.recipes[i].length)await post('/api/recipes?m='+m,ser(o.recipes[i]))}
  toast('Backup restored');await load()}catch(e){toast('Restore failed: '+e.message,1)}};
 rd.readAsText(f);ev.target.value=''}
async function rest(){if(!confirm('Restore the built-in recipes? All recipe changes will be lost.'))return;
 try{await post('/api/restore','');toast('Default recipes restored');await load()}catch(e){toast('Failed: '+e.message,1)}}
function tab(t){if(t=='hist')loadHist();['rec','set','hist','bak'].forEach(x=>{$(x).style.display=x==t?'block':'none';$('t_'+x).className=x==t?'on':''});$('bar').style.display=(t=='rec'&&dirty)?'flex':'none'}
load();
</script></body></html>)BBHTML";
