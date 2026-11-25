// Upright miniature notebook, B. Millimeters. Print "assembly" at Z=0.
// Two full annular dovetail pairs, separated by an open cage.
part = "assembly"; // [assembly,left,right]
$fn = 120;
stem = 3;
rise = 1.7;
gap = .55;                 // radial, measured at equal height
relief = .3;               // first-layer relief on each race surface
pitch = 23;
band = 9.4;
outer = 6.9;
panel_end = 39;
panel_y = 1;
thick = 3;
ring_top = pitch + band;
spoke_z = ring_top + .8;
top = spoke_z + 2.4;
assert(rise > gap && outer-stem-rise-gap > 1.2);
module turned(p) { rotate_extrude() polygon(p); }
// Rounded upright cover. The virtual outline extends 1.9 mm below the
// bed so the lower arcs start with printable slopes and a flat contact.
corner = 4.8;
edge_roll = .74;
module panel(x0,x1,h) {
    drop = 1.9;
    // Loft inset XZ perimeters over the two face-edge quarter rounds.
    function profile(a,side) = let(inset=edge_roll*(1-cos(a)),
        rad=corner-inset,
        y=side<0 ? panel_y+edge_roll*(1-sin(a))
                 : panel_y+thick-edge_roll*(1-sin(a)))
        [for(q=[0:3],j=[0:16]) let(t=q*90+j*90/16,
            cx=(q==0||q==3) ? x1-corner : x0+corner,
            cz=q<2 ? h-corner : -drop+corner)
            [cx+rad*cos(t),y,cz+rad*sin(t)]];
    loops = concat([for(a=[90:-15:0]) profile(a,-1)],
                   [for(a=[0:15:90]) profile(a,1)]);
    n = len(loops[0]);
    m = len(loops);
    // hull of closed perimeter samples gives planar faces at both ends.
    intersection() {
        polyhedron(points=[for(loop=loops) each loop],
            faces=concat([[for(i=[n-1:-1:0]) i]],
                [for(k=[0:m-2],i=[0:n-1]) let(j=(i+1)%n)
                    [k*n+i,k*n+j,(k+1)*n+j,(k+1)*n+i]],
                [[for(i=[0:n-1]) (m-1)*n+i]]),convexity=6);
        translate([x0-1,0,0]) cube([x1-x0+2,6,h+1]);
    }
}
module rounded_spoke() {
    // Flat underside for bridging, rounded plan ends and rolled upper lip.
    hull() for(a=[0:15:90]) {
        d=.47*(1-cos(a));
        translate([0,0,top-.47+.47*sin(a)-.01])
            linear_extrude(.01) offset(r=.8-d)
                translate([-13+.8,1+.8]) square([14-1.6,2-1.6]);
        if(a==0) translate([0,0,spoke_z]) linear_extrude(.02)
            offset(r=.8) translate([-13+.8,1+.8]) square([14-1.6,2-1.6]);
    }
}
// One circumferential male ridge or matching groove cutter.
// The socket cutter also opens the ring ends.
module bearing(z, socket=false, bed=false) {
    r = stem + (socket ? gap : 0);
    toe = bed ? (socket ? relief : -relief) : 0;
    translate([0,0,z]) turned(concat(
        socket ? [[0,-.05],[r+toe,-.05]] : [[0,0]],
        [[r+toe,0],[r,.4],[r,2.6],[r+rise,2.6+rise],
         [r+rise,5.2],[r,5.2+rise],[r,band+(socket ? .05 : 0)],
         [0,band+(socket ? .05 : 0)]]));
}
module fixed_web() {
    // A square inter-bearing web transmits torque from the lower boss.
    // Lead-ins support the changes from round to square section.
    hull() {
        translate([0,0,band-.1]) cylinder(h=.1,r=stem);
        translate([-2.4,-2.4,band+.9]) cube([4.8,4.8,.1]);
    }
    translate([-2.4,-2.4,band+.9]) cube([4.8,4.8,pitch-band-1.8]);
    hull() {
        translate([-2.4,-2.4,pitch-1]) cube([4.8,4.8,.1]);
        translate([0,0,pitch]) cylinder(h=.1,r=stem);
    }
}
module left_leaf() {
    difference() {
        union() {
            bearing(0,bed=true);
            fixed_web();
            bearing(pitch);
            turned(concat([[0,ring_top-.1],[stem,ring_top-.1]],
                [for(a=[0:10:90]) [stem-.65+.65*cos(a),top-.65+.65*sin(a)]],[[0,top]]));
            panel(-panel_end,-8,top);
            rounded_spoke();
        }
        // Exterior cover inscriptions. Shallow blind cuts keep the 
        // longest unsupported return across the recess below 0.5 mm.
        for (line=[["B",8.8,25.4],["Astra 6.2",3.7,16.5],
                   ["2026-11-24",3.4,9.8]])
            translate([-(8+panel_end)/2,4.01,line[2]]) rotate([90,0,180])
                translate([0,0,-.47]) linear_extrude(.5)
                    text(line[0],size=line[1],halign="center",valign="center",
                         font="Liberation Sans:style=Bold", $fn=32);
    }
}
module window() {
    // Tangential width shrinks one mm per mm of Z at the peaked roof.
    // Slight overlap at the ends avoids knife-edge intersections.
    translate([0,0,(pitch+band)/2]) rotate([90,0,90])
        linear_extrude(9)
            offset(r=.65,$fn=32) offset(delta=-.65) polygon([[-3,-3.6],[0,-6.6],[3,-3.6],
                     [3,3.6],[0,6.6],[-3,3.6]]);
}
module collar_cage() {
    difference() {
        union() {
            turned(concat([[0,0],[outer-.3,0],[outer,.4]],
                [for(a=[0:10:90]) [outer-.85+.85*cos(a),ring_top-.85+.85*sin(a)]],
                [[0,ring_top]]));
            panel(4,panel_end,ring_top);
        }
        bearing(0,socket=true,bed=true);
        bearing(pitch,socket=true);
        // Flared intermediate cavity makes the two bearing bands distinct.
        turned([[0,band-.02],[stem+gap,band-.02],[4.8,band+1.25],
                [4.8,pitch-1.25],[stem+gap,pitch+.02],[0,pitch+.02]]);
        for (a=[45:90:315]) rotate([0,0,a]) window();
    }
}
if(part=="assembly" || part=="left") left_leaf();
if(part=="assembly" || part=="right") collar_cage();
