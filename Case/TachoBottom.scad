$fn = 100;

BL = 66.8;
BH = 27.4;
WT = 2.4;

DL = 53.4;//50.3+0.2;
DH = 19+0.6;

// Top halw height
H = WT+5.6;

// Screw Peg height
IH = H + 7.8; // 3 for cover

// Radius
R = 3;

// Screws
SW = 60.2;
SH = 21.6;

difference()
{
  union()
  {
    difference()
    {
      union()
      {
        RoundBoxDown(BL+WT*2,BH+WT*2,H-WT/2,R);
        translate([0,0,H-WT/2]) RoundBoxDown(BL+WT,BH+WT,WT/2,R);
      }
      translate([0,0,WT]) RoundBoxDown(BL,BH,H*2,WT/2);
      // Cable
      translate([0,0,H-WT/2]) rotate([0,90,0]) cylinder(d=5.7, h=BL);
    }
    // Screw pegs
    translate([-SW/2,-SH/2,WT]) cylinder(d=5.4, h=IH-WT);
    translate([-SW/2,+SH/2,WT]) cylinder(d=5.4, h=IH-WT);
    translate([+SW/2,-SH/2,WT]) cylinder(d=5.4, h=IH-WT);
    translate([+SW/2,+SH/2,WT]) cylinder(d=5.4, h=IH-WT);
  }
  // Screw pegs
  translate([-SW/2,-SH/2,0.6]) ScrewHoleUp(100);
  translate([-SW/2,+SH/2,0.6]) ScrewHoleUp(100);
  translate([+SW/2,-SH/2,0.6]) ScrewHoleUp(100);
  translate([+SW/2,+SH/2,0.6]) ScrewHoleUp(100);
  // Magnets
  translate([-10/2+20,-20.6/2,0.6]) cube([10,20.6, 2.6]);
  translate([-10/2-20,-20.6/2,0.6]) cube([10,20.6, 2.6]);
}

module ScrewHole(h)
{
 translate([0,0,-1]) cylinder(d=3, h = h+2);
 translate([0,0,h-2.4]) cylinder(d2=6, d1=3, h = 2);
 translate([0,0,h-0.401]) cylinder(d=6, h = h);
}

module ScrewHoleUp(h)
{
 translate([0,0,2-0.001]) cylinder(d=3, h = h+2);
 translate([0,0,0]) cylinder(d2=3, d1=6, h = 2);
 translate([0,0,-h+0.001]) cylinder(d=6, h = h);
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