$fn = 100;

// Board length
BL = 66.8;
// Board height
BH = 27.4;
// Wall thickness
WT = 2.4;

// Display length
DL = 53.4;//50.3+0.2;
// Display height
DH = 19+0.6;

// Screw Peg height
IH = 8+3; // 3 for cover

// Top half height
H = 22;

// Radius
R = 3;

// Screws
SW = 60.2;
SH = 21.6;

// Case
difference()
{
  // Case
  FilletBoxDown(BL+WT*2,BH+WT*2,H,R);
  // Lock
  translate([0,0,H-WT/2]) RoundBoxDown(BL+WT,BH+WT,H*2,R-WT/2);
  // Inside
  translate([0,0,WT]) RoundBoxDown(BL,BH,H*2,WT/2);
  // Display
  translate([-DL/2,-DH/2,-1]) cube([DL,DH,H*2]);
  // Cable
  translate([0,0,H]) rotate([0,90,0]) cylinder(d=5.7, h=BL);
}
// Case inside structure
difference()
{
  union()
  {
    // Display border
    translate([-DL/2-WT,-DH/2-WT,0]) cube([DL+WT*2,DH+WT*2,IH]);
    // Screw pegs
    translate([-SW/2,-SH/2,WT]) cylinder(d=8, h=IH-WT);
    translate([-SW/2,+SH/2,WT]) cylinder(d=8, h=IH-WT);
    translate([+SW/2,-SH/2,WT]) cylinder(d=8, h=IH-WT);
    translate([+SW/2,+SH/2,WT]) cylinder(d=8, h=IH-WT);
  }
  // Display border
  translate([-DL/2,-DH/2,-1]) cube([DL,DH,H*2]);
  // Cut for soldered pins
  translate([-DL/2-WT*2,-DH/4,IH-2]) cube([DL+WT*4,DH/2,H*2]);
  // Screw pegs
  translate([-SW/2,-SH/2,WT]) cylinder(d=2.8, h=IH);
  translate([-SW/2,+SH/2,WT]) cylinder(d=2.8, h=IH);
  translate([+SW/2,-SH/2,WT]) cylinder(d=2.8, h=IH);
  translate([+SW/2,+SH/2,WT]) cylinder(d=2.8, h=IH);
}

module ScrewPeg(H)
{
  difference()
  {
    cylinder(d=8, h=H);
    cylinder(d=2.8, h=H+1);
  }
}

module RoundBoxDown(L,W,H,R)
{
  hull()
  {
    translate([-L/2+R,-W/2+R,0]) cylinder(r=R, h=H);
    translate([-L/2+R,+W/2-R,0]) cylinder(r=R, h=H);
    translate([+L/2-R,-W/2+R,0]) cylinder(r=R, h=H);
    translate([+L/2-R,+W/2-R,0]) cylinder(r=R, h=H);
  }
}

module FilletBoxDown(L,W,H,R)
{
  // Cubes
  translate([-L/2+R,-W/2+R,0])  cube([L-R*2,W-R*2,H]);
  translate([-L/2,  -W/2+R,R]) cube([L,W-R*2,H-R]);
  translate([-L/2+R,-W/2,  R]) cube([L-R*2,W,H-R]);
  // Corner shperes
  translate([-L/2+R,-W/2+R,R]) sphere(r=R);
  translate([-L/2+R,+W/2-R,R]) sphere(r=R);
  translate([+L/2-R,-W/2+R,R]) sphere(r=R);
  translate([+L/2-R,+W/2-R,R]) sphere(r=R);
  // Corner cylinders
  translate([-L/2+R,-W/2+R,R]) cylinder(r=R, h=H-R);
  translate([-L/2+R,+W/2-R,R]) cylinder(r=R, h=H-R);
  translate([+L/2-R,-W/2+R,R]) cylinder(r=R, h=H-R);
  translate([+L/2-R,+W/2-R,R]) cylinder(r=R, h=H-R);
  // Top cylinders
  translate([-L/2+R,-W/2+R,R]) rotate([0,90,0]) cylinder(r=R, h=L-2*R);
  translate([-L/2+R,-W/2+R,R]) rotate([-90,0,0]) cylinder(r=R, h=W-2*R);
  translate([-L/2+R,+W/2-R,R]) rotate([0,90,0]) cylinder(r=R, h=L-2*R);
  translate([+L/2-R,-W/2+R,R]) rotate([-90,0,0]) cylinder(r=R, h=W-2*R);
}