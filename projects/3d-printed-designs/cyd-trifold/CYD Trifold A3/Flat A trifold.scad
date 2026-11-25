// Four separately printed-in-place A cartridges. All ten print objects lie flat.
include <A kernel.scad>
part="tray"; which=1; // tray,face,cartridge,cartridge-left,cartridge-right,front-half,back-half
station=60; side="left";
centers=[-54,0,54]; front_z=16; rear_z=3.8; case_end=74;
module box(a,b){translate(a)cube(b-a);}
module rbox(a,b,r=.7){hull()for(x=[a.x+r,b.x-r],y=[a.y+r,b.y-r])translate([x,y,a.z])cylinder(h=b.z-a.z,r=r);}
module mirrorx(s){if(s<0)mirror([1,0,0])children();else children();}
module mirrory(s){if(s<0)mirror([0,1,0])children();else children();}
module placed_half(seam,yy,half){
 translate([seam==0?-27:27,yy,seam==0?front_z:rear_z])
 if(seam==0){if(half==0)left();else right();}
 else rotate([180,0,0]){if(half==0)left();else right();}
}
module pedestal(local_x,yy,half,zmax){
 // Supports only the outboard tip of the original foot, beyond USB insertion corridor.
 ylen=half==0?13:8;
 for(s=[-1,1])if((yy>0&&s>0)||(yy<0&&s<0)){
 a=yy+s*(ylen-2);b=yy+s*ylen;
 box([local_x-1.5,min(a,b),0],[local_x+1.5,max(a,b),zmax]);
 box([local_x-1.5,min(a,yy>0?case_end-3:-case_end),0],[local_x+1.5,max(b,yy>0?case_end:-case_end+3),2]);
 }
}
module foot_socket(seam,yy,half){
 // Back foot pocket on flat floor; body clearances remain unchanged.
 xax=27; x0=half==0?-26:7; x1=half==0?-7:26; yl=half==0?13:8;
 translate([xax,yy,0])difference(){
 rbox([x0-.8,-yl-.6,0],[x1+.8,yl+.6,4.0],.6);
 box([x0-.2,-yl-.2,.6],[x1+.2,yl+.2,4.2]);
 // The existing cone/cheek/hub occupies the inner portion; keep only outer-foot cradle.
 if(half==0)box([-16,-20,-.1],[10,20,5]);else box([-10,-20,-.1],[16,20,5]);
 }
 xa=half==0?x0-.8:16; xb=half==0?-16:x1+.8;
 if(yy>0)box([27+xa,yy+yl,0],[27+xb,case_end,.6]);
 else box([27+xa,-case_end,0],[27+xb,yy-yl,.6]);
}
module tray(i){
 x=centers[i];
 difference(){union(){
 // Fully open rear and long-side service slots; no solid wall across buttons or headers.
 translate([x,0,0]){
 for(s=[-1,1])mirrorx(s){
 rbox([25.3,-case_end,0],[26.6,case_end,2],.5);
 for(yy=[-41.5,41.5]){
 rbox([25.3,yy-2.5,0],[26.6,yy+2.5,18.35],.5);
 rbox([20.8,yy-2,0],[25.35,yy+2,12],.5);
 // Pad tie joins the corner seat to its edge rail, leaving most PCB rear unobstructed.
 }
 }
 for(s=[-1,1])mirrory(s){rbox([-26.6,case_end-3,0],[26.6,case_end,2],.7);rbox([-26.6,44,0],[26.6,46,2],.7);}
 }
 // Front cartridge: original feet rest on outboard-only supports.
 if(i==0)for(yy=[-station,station])pedestal(-27-8.5,yy,0,15.8);
 if(i==1)for(yy=[-station,station])pedestal(-27+8.5,yy,1,15.8);
 // Rear cartridges captured in matching rear-tray mortises.
 if(i==1)for(yy=[-station,station])foot_socket(1,yy,0);
 if(i==2)for(yy=[-station,station])foot_socket(1,yy,1);
 }
 // Rear foot mortises also remove material from the shared outboard crossbar.
 if(i==1||i==2)for(yy=[-station,station]){half=i==1?0:1;xl=half==0?1:34;xh=half==0?20:53;yl=half==0?13:8;box([xl-.2,yy-yl-.2,.6],[xh+.2,yy+yl+.2,4.2]);}
 // Rear cartridge kernel has a dedicated free-sweep bay; preserve inner tie bars.
 if(i==1||i==2)for(yy=[-station,station])box([13,yy-13.5,-5],[41,yy+13.5,9]);
 // Front bezel corner-clip sockets: no permanent seated solid overlap.
 for(s=[-1,1],yy=[-41.5,41.5])translate([x,0,0])mirrorx(s){
 box([25.15,yy-1.6,4.6],[26.8,yy+1.6,18.6]);
 box([25.75,yy-1.6,2],[26.8,yy+1.6,18.6]);
 box([25.15,yy-1.35,2.3],[25.85,yy+1.35,3.8]);
 }
 }
}
module bezel_plate(i,z0,z1){
 hull()for(s=[-1,1],t=[-1,1]){rr=((i==0&&s<0)||(i==2&&s>0))?3:.8;
 translate([s*(26.6-rr),t*(case_end-rr),z0])cylinder(h=z1-z0,r=rr);}
}
module face_clip(s,yy){mirrorx(s){
 box([25.9,yy-1.1,2.5],[26.6,yy+1.1,18.6]);
 translate([0,yy+1.1,0])rotate([90,0,0])linear_extrude(2.2)
 polygon([[26,2.5],[25.3,3.0],[25.3,3.4],[26,3.6]]);
}}
module front_pocket(yy,half){
 xax=-27;x0=half==0?-26:7;x1=half==0?-7:26;yl=half==0?13:8;
 translate([xax,yy,0])box([x0-.2,-yl-.2,15.6],[x1+.2,yl+.2,19.2]);
}
module face(i){x=centers[i];
 difference(){union(){
 translate([x,0,0]){
 difference(){bezel_plate(i,18.45,19.75);rbox([-22,-31.7,18],[22,26.7,20],.8);}
 for(s=[-1,1],yy=[-41.5,41.5])face_clip(s,yy);
 for(s=[-1,1],yy=[-39,39])mirrorx(s)rbox([20.8,yy-1.8,13.7],[23.8,yy+1.8,18.6],.5);
 }
 // Thick face-down printed front foot mortises; open on their back side.
 if(i==0||i==1)for(yy=[-station,station]){
 half=i==0?0:1;x0=half==0?-26:7;x1=half==0?-7:26;yl=half==0?13:8;
 translate([-27,yy,0])rbox([x0-1,-yl-.7,15.8],[x1+1,yl+.7,19.75],.6);
 }
 // Rear foot hold-down stops at outboard tips, clear of the connector corridor.
 if(i==1||i==2)for(yy=[-station,station]){
 half=i==1?0:1;xp=half==0?27-25:27+25;yl=half==0?13:8;
 ya=yy+(yy>0?yl-2:-yl);yb=yy+(yy>0?yl:-yl+2);
 box([xp-2,ya,4.0],[xp+2,yb,18.6]);
 }
 }
 if(i==0||i==1)for(yy=[-station,station]){
 front_pocket(yy,i==0?0:1);
 // Preserve a generous free sweep around ALL original A bearing features.
 box([-43,yy-14,15],[-11,yy+14,26]);
 }
 }
}
if(part=="tray")tray(which);
if(part=="face")face(which);
if(part=="cartridge"){left();right();}
if(part=="cartridge-left")left();if(part=="cartridge-right")right();
if(part=="front-half")placed_half(0,station,which);
if(part=="back-half")placed_half(1,station,which);
