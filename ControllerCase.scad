//Render plastic protection piece
//TopPlate($DoMountHoles = 0);

//Render laser cuttable structure pieces
//CutGroup1();
//CutGroup2();
//CutGroup3();

//Render 3D model
3DModel();

//Box dimensions
$Width = 365;
$Height = 267;
$Depth = 50;
$ButtonOffsetX = 25;
$ButtonOffsetY = -10;

$ButtonHoles = [[5, 42], [50, 42], [90, 42], [5, 129], [5, 216],[90, 182], [67, 223], [131, 72], [131, 122], [131, 172], [169, 91], [169, 141], [169, 191], [286, 26], [274, 61], [252, 97], [230, 138], [240, 180]];

//Size of overlap 'edge'. Set to zero for box with no lip. Do not recommend using tslots with zero overlap
$Overlap = 4;

//Rounding radius of the corners
$Rounding = 8;

//Thickness of the material being used
$MaterialThickness = 5;

//Gap between pieces when laser cutting
$CutGap = 3;

//T-slot bolt parameters
//Set 'DoxxxxTSlots = 1' for bolted box, set = 0 for glued box
//Suggest top = 0, bottom = 1 for glued top, t-slotted bottom
$DoTopTSlots = 0;
$DoBottomTSlots = 1;
$MountBoltDiameter = 3;
$MountBoltSlotLength = 15;
$MountBoltLocation = 10;
//T-slot nut parameters
$TNutWidth = 5.6;
$TNutHeight = 2.1;

//Button hole parameters
$SmallButtonDiameter = 30;//24.3;
$LargeButtonDiameter = 30;//23.7;

//Joystick mounting hole parameters
$JoystickMountHoleDiameter = 2.5;
$JoystickHSpacing = 43;
$JoystickVSpacing = 81;

//3D display colors
$ColorTop = [193/255, 141/255, 7/255];
$ColorBottom = [173/255, 121/255, 0/255];
$ColorFrontBack = [204/255, 174/255, 69/255];
$ColorLeftRight = [194/255, 164/255, 59/255];

$fn = 100;

module Slots()
{
  $SegmentLength = ($Length - $MaterialThickness - $MaterialThickness) / 6;
  translate([$SegmentLength + $MaterialThickness, 0])
    square([$SegmentLength, $MaterialThickness]);
  translate([($SegmentLength * 4) + $MaterialThickness, 0])
    square([$SegmentLength, $MaterialThickness]);
}

module Pegs()
{
  $SegmentLength = ($Length - $MaterialThickness - $MaterialThickness) / 6;
  square([$SegmentLength + $MaterialThickness, $MaterialThickness]);
  
  translate([($SegmentLength * 2) + $MaterialThickness,0])
    square([$SegmentLength * 2, $MaterialThickness]);
  translate([($SegmentLength * 5) + $MaterialThickness,0])
    square([$SegmentLength + $MaterialThickness, $MaterialThickness]);
}

module SlotsPegs()
{
  if ($Type == 0)
    Slots();
  else
    Pegs();
}

module MainPlate()
{
  difference()
  {
    translate([$Rounding, $Rounding])
    minkowski()
    {
      circle(r = $Rounding);
      square([$Width - $Rounding - $Rounding, $Height - $Rounding - $Rounding]);
    }
    if ($DoMountHoles == 1)
    {
    //Slots on lower edge of top plate
    translate([0, $Overlap])
      Slots($Length = $Width);
    //Slots on upper edge of top plate
    translate([0, $Height - $MaterialThickness - $Overlap])
      Slots($Length = $Width);
    //Slots on left edge of top plate
    translate([$MaterialThickness + $Overlap, 0])
      rotate(90, [0, 0, 1])
        Slots($Length = $Height);
    //Slots on right edge of top plate
    translate([$Width - $Overlap, 0])
      rotate(90, [0, 0, 1])
        Slots($Length = $Height);
    }
    //TSlot mounting holes
    if ($DoTSlots == 1)
    {
      translate([$Width / 2, $Overlap + ($MaterialThickness / 2)])
        circle(d = $MountBoltDiameter);
      translate([$Width / 2, $Height - $Overlap - ($MaterialThickness / 2)])
        circle(d = $MountBoltDiameter);
      translate([$Overlap + ($MaterialThickness / 2), $Height / 2])
        circle(d = $MountBoltDiameter);
      translate([$Width - $Overlap - ($MaterialThickness / 2), $Height / 2])
        circle(d = $MountBoltDiameter);
    }
  }
}

module TopPlate()
{
  difference()
  {
    MainPlate($DoMountHoles = $DoMountHoles, $DoTSlots = $DoTopTSlots);
	translate([$ButtonOffsetX, $ButtonOffsetY, 0])
		RenderHoles($Locations = $ButtonHoles, $Diameter = 30);

  }
}

module BottomPlate()
{
	MainPlate($DoMountHoles = 1, $DoTSlots = $DoBottomTSlots);
}

module TSlot()
{
  translate([- $MountBoltDiameter / 2, 0])
  {
    square([$MountBoltDiameter, $MountBoltSlotLength]);
    translate([-($TNutWidth - $MountBoltDiameter) / 2, $MountBoltLocation])
      square([$TNutWidth, $TNutHeight]);
  }
}

module Support()
{
  difference()
  {
    translate([$Overlap, 0])
      square([$Length - $Overlap - $Overlap, $Depth]);
    //Lower pegs
    //Translate a little to avoid zero width wals
    translate([0, -0.01])
      Pegs();
    //Upper pegs
    translate([0, $Depth - $MaterialThickness + 0.01])
      Pegs();
    //Left pegs
    translate([$MaterialThickness + $Overlap - 0.01, 0])
      rotate(90, [0, 0, 1])
        SlotsPegs($Length = $Depth, $Type = $Type);
    //Right slots
    translate([$Length - $Overlap + 0.01, 0])
      rotate(90, [0, 0, 1])
        SlotsPegs($Length = $Depth, $Type = $Type);
    if ($DoBottomTSlots == 1)
    {
      //T-slots
      translate([$Length / 2, 0])
        TSlot();
    }
    if ($DoTopTSlots == 1)
    {
      translate([$Length / 2, $Depth])
      rotate(180, [0, 0, 1])
        TSlot();
    }
  }
}

module BackSupport()
{
  difference()
  {
    Support();
    //USB cable opening
    hull()
    {
      translate([150, $MaterialThickness])
        circle(d = 5);
      translate([150, $MaterialThickness + 2.5])
        circle(d = 5);
    }
  }
}

module CutGroup1()
{
  //Top plate
  //Mirror so burn marks are on inside
  translate([$Width, 0, 0])
    mirror([1, 0, 0])
      TopPlate($DoMountHoles = 1);
}

module CutGroup2()
{
  //Bottom plate
	BottomPlate();
}

module CutGroup3()
{
  //Front support
	Support($Length = $Width, $Type = 0);
  //Back support
  translate([0, $Depth + $CutGap])
    BackSupport($Length = $Width, $Type = 0);
  //Left support
  translate([0, $CutGap + $CutGap + $Depth + $Depth])
      Support($Length = $Height, $Type = 1);
  //Right support
  translate([0, $CutGap + $CutGap + $CutGap + $Depth  + $Depth + $Depth])
      Support($Length = $Height, $Type = 1);
}

module 3DTop()
{
  color($ColorTop)
    linear_extrude(height = $MaterialThickness)
      TopPlate($DoMountHoles = 1);
}

module 3DBottom()
{
  color($ColorBottom)
    linear_extrude(height = $MaterialThickness)
      BottomPlate();
}

module 3DLeft()
{
  color($ColorLeftRight)
    rotate(90, [0, 0, 1])
      rotate(90, [1, 0, 0])
        linear_extrude(height = $MaterialThickness)
          Support($Length = $Height, $Type = 1);
}

module 3DRight()
{
  color($ColorLeftRight)
    rotate(90, [0, 0, 1])
      rotate(90, [1, 0, 0])
        linear_extrude(height = $MaterialThickness)
          Support($Length = $Height, $Type = 1);
}

module 3DFront()
{
  color($ColorFrontBack)
    rotate(90, [1, 0, 0])
      linear_extrude(height = $MaterialThickness)
        Support($Length = $Width, $Type = 0);
}

module 3DBack()
{
  color($ColorFrontBack)
    rotate(90, [1, 0, 0])
      linear_extrude(height = $MaterialThickness)
        BackSupport($Length = $Width, $Type = 0);
}

module 3DButton()
{
  rotate_extrude(convexity = 10, $fn = 100)
    translate([$LargeButtonDiameter / 2, 0, 0])
    circle(r = 2, $fn = 100);
  translate([0, 0, 1])
    scale([1, 1, .2])
      sphere(d = $LargeButtonDiameter);
}

module 3DModel()
{
  //Top piece
  translate([0, 0, $Depth - $MaterialThickness])
	3DTop();
  //Bottom piece
	3DBottom();
  //Left side
	translate([$Overlap, 0, 0])
  3DLeft();
  //Right side
  translate([$Width - $MaterialThickness - $Overlap, 0, 0])
    3DRight();
  //Front side
  translate([0, $MaterialThickness + $Overlap, 0])
    3DFront();
  //Back side
  translate([0, $Height - $Overlap , 0])
    3DBack();
	
  translate([$ButtonOffsetX, $ButtonOffsetY, $Depth])
		color("RED")
		Render3DButtons($Locations = $ButtonHoles);
}

module RenderHoles()
{
	for($HoleCoord=$Locations)
	{
		translate([$HoleCoord[0], $Height - $HoleCoord[1], 0])
			//rotate(-90, [1, 0, 0])
				circle(d = $Diameter, $fn = 150, center = true);
	}
}

module Render3DButtons()
{
	for($HoleCoord=$Locations)
	{
		translate([$HoleCoord[0], $Height - $HoleCoord[1], 0])
			//rotate(-90, [1, 0, 0])
				3DButton();
	}
}


