// Render local SVG assets with Chromium; export a GitHub-compatible GIF fallback.
const {chromium}=require('playwright');
const fs=require('fs');const path=require('path');
(async()=>{
 const root=path.resolve(__dirname,'..');
 const executable=[process.env.BATTERY_FACES_CHROME,chromium.executablePath(),'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'].find(p=>p&&fs.existsSync(p));
 if(!executable)throw new Error('Install Playwright Chromium or set BATTERY_FACES_CHROME');
 const browser=await chromium.launch({headless:true,executablePath:executable});
 const page=await browser.newPage({deviceScaleFactor:1});
 for(const name of ['battery-faces-family','blue-board','orange-board','yellow-board','rainbow-board','purple-board','pink-board','teal-board','tab5-board']){
  const svg=path.join(root,'assets/renders',name+'.svg');
  const source=fs.readFileSync(svg,'utf8');const m=source.match(/width="(\d+)" height="(\d+)"/);
  await page.setViewportSize({width:Number(m[1]),height:Number(m[2])});
  await page.goto('file://'+svg);if(await page.locator('parsererror').count())throw new Error('Invalid SVG: '+svg);await page.screenshot({path:svg.replace(/\.svg$/,'.png'),omitBackground:true});
 }
 await page.setViewportSize({width:660,height:470});
 await page.goto('file://'+path.join(root,'assets/animation/battery-mood.svg'));
 for(let i=0;i<5;i++){
  await page.evaluate(t=>{const svg=document.querySelector('svg');svg.pauseAnimations();svg.setCurrentTime(t);},i*2+.1);
  await page.screenshot({path:path.join(root,`assets/animation/mood-${i}.png`)});
 }
 await browser.close();
})().catch(e=>{console.error(e);process.exit(1);});
