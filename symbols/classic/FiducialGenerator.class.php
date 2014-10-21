<?php
	/*
	* Fiducial Image generating class in PHP with GD
	* 
	* This PHP script generates unique images which can be used for fiducial tracking.
	* Make sure the './' directory is set to writable for PHP
	* GD is needed, but most servers come with it installed these days.
	* The drawing code was done by Bram de Jong, the function that
	* generates the unique sequences was done by Ross bencina
	* 
	* @author Bram de Jong <bram@Smartelectronix.com>
	* @author Ross Bencina <rossb@audiomulch.com>
	* @version 2004_10_04
	*/

	class FiducialGenerator
	{
		// pixels of images, change as wanted
		var $numbers = array(
						array(	1,0,0,0,1,
							1,1,1,1,1,
							1,0,0,0,0	 ),
	
						array(	1,1,1,0,1,
							1,0,0,0,1,
							1,0,1,1,1	 ),
	
						array(	1,0,1,0,1,
							1,0,1,0,1,
							1,0,1,0,1	 ),
	
						array(	0,0,1,0,1,
							0,0,1,0,0,
							1,0,1,0,1	 ),
	
						array(	1,0,1,1,1,
							0,0,0,0,0,
							1,0,1,0,1	 ),
	
						array(	1,0,1,0,1,
							0,0,0,0,0,
							1,0,1,0,1	 )
					 );
					 
		// image things
		var $image;
		var $width;
		var $height;
		var $black;
		var $white;
	
		// number of pixels per graphical element
		var $pixelWidth;
		
		// array of fiducial sequences
		var $fiducials;
		
		// these should be const, but how the hell d'u do that in a class ?
		var $N_FIDUCIALS = 120;
		var $FIDUCIAL_WIDTH = 21;

		function FiducialGenerator()
		{
			// load sequences into array
			$this->loadFiducials();
		}
		
		// this should be the only public function, but PHP 4.0 doesn't know public/private
		function generatePages($pixelWidth = 5, $nHorizontal = 8, $nVertical = 5)
		{
			$this->pixelWidth = $pixelWidth;
			
			// padding for cutting lines
			$paddingVertical = $paddingHorizontal = 1;
			
			$this->width = ($this->pixelWidth * 23 + $paddingHorizontal) * $nHorizontal + $paddingHorizontal;
			$this->height = ($this->pixelWidth * 21 + $paddingVertical) * $nVertical + $paddingVertical;
			
			$this->image = imagecreate($this->width,$this->height);
			$this->black = ImageColorAllocate($this->image,0,0,0);
			$this->white = ImageColorAllocate($this->image,255,255,255);

			$nPages = (integer) ceil($this->N_FIDUCIALS / ($nHorizontal * $nVertical));
			$nGenerated = 0;

			for($pages=0;$pages<$nPages;$pages++)
			{
				imagefilledrectangle($this->image,0,0,$this->width,$this->height,$this->white);
				
				for($y=0;$y<$nVertical;$y++)
				{
					for($x=0;$x<$nHorizontal;$x++)
					{
						$dx = $paddingHorizontal + $x * 23 * $pixelWidth + $paddingHorizontal * $x;
						$dy = $paddingVertical + $y * 21 * $pixelWidth + $paddingVertical * $y;
						
						// cut markers
						$this->relativeLine($dx - $paddingHorizontal - $pixelWidth,$dy - $paddingVertical,
											$dx - $paddingHorizontal + $pixelWidth,$dy - $paddingVertical,
											$this->black);
					
						$this->relativeLine($dx - $paddingHorizontal,$dy - $paddingVertical - $pixelWidth,
											$dx - $paddingHorizontal,$dy - $paddingVertical + $pixelWidth,
											$this->black);
						
						if($nGenerated < 120)
						{
							$this->generateOneFiducial($this->fiducials[$nGenerated],$nGenerated,$dx,$dy);
							$nGenerated++;
						}
					}
				}
				
				// cut markers
				for($y=0;$y<$nVertical;$y++)
				{				
					$dx = $paddingHorizontal + $nHorizontal * 23 * $pixelWidth + $paddingHorizontal * $nHorizontal;
					$dy = $paddingVertical + $y * 21 * $pixelWidth + $paddingVertical * $y;
										
					$this->relativeLine($dx - $paddingHorizontal - $pixelWidth,$dy - $paddingVertical,
										$dx - $paddingHorizontal + $pixelWidth,$dy - $paddingVertical,
										$this->black);
				
					$this->relativeLine($dx - $paddingHorizontal,$dy - $paddingVertical - $pixelWidth,
										$dx - $paddingHorizontal,$dy - $paddingVertical + $pixelWidth,
										$this->black);
				}

				for($x=0;$x<$nHorizontal+1;$x++)
				{
					$dx = $paddingHorizontal + $x * 23 * $pixelWidth + $paddingHorizontal * $x;
					$dy = $paddingVertical + $nVertical * 21 * $pixelWidth + $paddingVertical * $nVertical;
					
					$this->relativeLine($dx - $paddingHorizontal - $pixelWidth,$dy - $paddingVertical,
										$dx - $paddingHorizontal + $pixelWidth,$dy - $paddingVertical,
										$this->black);
				
					$this->relativeLine($dx - $paddingHorizontal,$dy - $paddingVertical - $pixelWidth,
										$dx - $paddingHorizontal,$dy - $paddingVertical + $pixelWidth,
										$this->black);
				}
			
				// generate output
				$filename = sprintf("Sheet_%02d.png",$pages);
				ImagePng($this->image,$filename);
				
				echo "<img src=$filename><br><br>";
			}

			ImageDestroy($this->image);
		}
		
		// draws one structure
		function generateOneFiducial($fiducialArray, $tag, $dx, $dy)
		{
			$this->relativeRect(0,0,21,21,$this->white,$dx,$dy);
			$this->relativeRect(1,1,19,19,$this->black,$dx,$dy);
		
			$this->makeSub(2,2,$fiducialArray[0]-1,$dx,$dy);
			$this->makeSub(2,8,$fiducialArray[1]-1,$dx,$dy);
			$this->makeSub(2,14,$fiducialArray[2]-1,$dx,$dy);
			$this->makeSub(11,2,$fiducialArray[5]-1,$dx,$dy);
			$this->makeSub(11,8,$fiducialArray[4]-1,$dx,$dy);
			$this->makeSub(11,14,$fiducialArray[3]-1,$dx,$dy);
			
			$this->relativeString(21.3,4,$tag, $this->black,$dx,$dy);
		}

		// drawing of the "pixels" of one structure
		function makeSub($x,$y, $number,$dx,$dy)
		{
			$this->relativeRect($x,$y,8,5,$this->white,$dx,$dy);
		
			for($i=0;$i<5;$i++)
			{
				for($j=0;$j<3;$j++)
				{
					if($this->numbers[$number][$j*5 + $i] == 1)
						$this->setPixel($x+$i+1, $y+$j+1, $this->black,$dx,$dy);
				}
			}
		}

		function relativeLine($x1, $y1, $x2, $y2, $color, $dx = 0, $dy = 0)
		{
			imageline($this->image, $x1+$dx, $y1+$dy, $x2+$dx, $y2+$dy, $color);
		}
		
		function relativeRect($x, $y, $width, $height, $color, $dx = 0, $dy = 0)
		{
			ImageFilledRectangle($this->image,$x*$this->pixelWidth+$dx,$y*$this->pixelWidth+$dy,($x+$width)*$this->pixelWidth - 1 + $dx,($y+$height)*$this->pixelWidth - 1+$dy,$color);
		}
		
		function setPixel($x, $y, $color,$dx,$dy)
		{
			$this->relativeRect($x,$y,1,1,$color,$dx,$dy);
		}
	
		function relativeString($x, $y, $tag, $color, $dx = 0, $dy = 0)
		{
			imagestringup($this->image,1,$x*$this->pixelWidth + $dx, $y*$this->pixelWidth + $dy, $tag, $color);
		}
	
		// generate all permutations, function created by Ross Bencina
		function loadFiducials()
		{
			$a = 1;
			$b = 2;
			$c = 3;
			$d = 4;
			$e = 5;
			$f = 6;
		
			for( $b = 2; $b <= 6; ++$b ){
				for( $c = 2; $c <= 6; ++$c ){
					if( $c == $b ) continue;
					for( $d = 2; $d <= 6; ++$d ){
						if( $d == $b || $d == $c) continue;
						for( $e = 2; $e <= 6; ++$e ){
							if( $e == $b || $e == $c || $e == $d) continue;
								for( $f = 2; $f <= 6; ++$f ){
								if( $f == $b || $f == $c || $f == $d || $f == $e) continue;
								{
									$factorial = array( 24, 6, 2, 1, 1 );
									$mapping = array( 0, 1, 2, 3, 4 );
							
									// the algorithm uses zero based mappings from 0 to 4
									$input = array($b - 2, $c - 2, $d - 2, $e - 2, $f - 2);
							
									$id = 0;
									{
										for( $i=0; $i < 5; ++$i ){
											$id += $mapping[ $input[$i] ] * $factorial[$i];
											for( $j = $input[$i] + 1; $j < 5; ++$j ){
												$mapping[$j]--;
											}
										}
									}
							
									$fiducial = array($a, $b, $c, $d, $e, $f);
									$this->fiducials[$id] = $fiducial;
								}
							}
						}
					}
				}
			}
		}
	}
?>

<html>
<body bgcolor="#FB8C8C" text=black>
<BR><FORM method="post" action="index.php">
	pixelWidth: <input type="text" name="pixelWidth" value="5" size="2"><br>
	nHorizontal: <input type="text" name="nHorizontal" value="5" size="2"><br>
	nVertical : <input type="text" name="nVertical" value="8" size="2"><br>
	<input type="submit" name="submit" value="submit">
	</form><p>
<?
	if( isset($pixelWidth) && isset($nHorizontal) && isset($nVertical) )
	{
		$generator = new FiducialGenerator();
		$generator->generatePages($pixelWidth,$nHorizontal,$nVertical);
	}
?>
</body>
</html>
