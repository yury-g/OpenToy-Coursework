// Preview GitHub's rendered Markdown locally, without a logged-in browser profile.
const {chromium}=require('playwright'),fs=require('fs'),path=require('path'),{pathToFileURL}=require('url');
(async()=>{
 const root=path.resolve(__dirname,'..'),source=process.argv[2];if(!source)throw new Error('Pass GitHub markdown API HTML output');
 const exe=[process.env.BATTERY_FACES_CHROME,chromium.executablePath(),'/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'].find(p=>p&&fs.existsSync(p));
 const html='<html><head><meta charset="UTF-8"><base href="'+pathToFileURL(root+'/')+'"><style>body{background:#0d1117;color:#e6edf3;font:16px/1.6 -apple-system,Arial,sans-serif;margin:0}main{max-width:1000px;padding:32px;margin:auto}img{max-width:100%;height:auto}a{color:#58a6ff}h1,h2{border-bottom:1px solid #30363d;padding-bottom:.3em}table{border-collapse:collapse;width:100%}td,th{border:1px solid #30363d;padding:8px}code{background:#222b36;padding:2px 4px;border-radius:4px}pre{overflow:auto}</style></head><body><main>'+fs.readFileSync(source,'utf8')+'</main></body></html>';
 const local=path.join(root,'outputs/readme-preview-v17.html');fs.writeFileSync(local,html);
 const browser=await chromium.launch({headless:true,executablePath:exe}),page=await browser.newPage({viewport:{width:1100,height:900},deviceScaleFactor:1});
 await page.goto(pathToFileURL(local).href);await page.waitForFunction(()=>Array.from(document.images).every(i=>i.complete&&i.naturalWidth>0));
 await page.screenshot({path:path.join(root,'assets/renders/readme-preview.png'),fullPage:true});
 console.log('PASS README render: all',await page.locator('img').count(),'images loaded');await browser.close();
})().catch(e=>{console.error(e);process.exit(1);});
