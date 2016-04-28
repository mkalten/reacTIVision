import processing.pdf.*;
//import com.hamoid.*;

PFont font;
PShape fiducial;
PShape bits[] = new PShape[20];
PShape check[] = new PShape[4];

PGraphics pdf;
//VideoExport video;

// edit these three values
int start_id = 0;
int range = 300;
int fiducial_size_mm = 46;

String license = "reacTIVision yamaarashi symbols CC BY-NC-SA 2016 Martin Kaltenbrunner <martin@tuio.org>";
int border = 22;
int xpos = border;
int ypos = border;
int fiducial_size;
 
void setup() {
  
  size(480, 480);
  noLoop();
  
  //video = new VideoExport(this, "yamaarashi.mp4");
  //video.setFrameRate(60);  
  pdf = createGraphics(598, 842, PDF, "yamaarashi_"+start_id+"-"+(start_id+range-1)+".pdf");
  font = createFont("Arial", 8);
  
  fiducial_size = round(fiducial_size_mm * 2.83464567);
  fiducial = loadShape("yamaarashi.svg");
  
  // define bit order
  bits[0] = fiducial.getChild("bit1");
  bits[1] = fiducial.getChild("bit7");
  bits[2] = fiducial.getChild("bit13");
  bits[3] = fiducial.getChild("bit19");
  bits[4] = fiducial.getChild("bit2");
  bits[5] = fiducial.getChild("bit8");
  bits[6] = fiducial.getChild("bit14");
  bits[7] = fiducial.getChild("bit20");
  bits[8] = fiducial.getChild("bit3");
  bits[9] = fiducial.getChild("bit9");
  bits[10] = fiducial.getChild("bit15");
  bits[11] = fiducial.getChild("bit21");
  bits[12] = fiducial.getChild("bit4");
  bits[13] = fiducial.getChild("bit10");
  bits[14] = fiducial.getChild("bit16");
  bits[15] = fiducial.getChild("bit22");
  bits[16] = fiducial.getChild("bit5");
  bits[17] = fiducial.getChild("bit11");
  bits[18] = fiducial.getChild("bit17");
  bits[19] = fiducial.getChild("bit23");
  
  // checksum bits
  check[0] = fiducial.getChild("bit6");
  check[1] = fiducial.getChild("bit12");
  check[2] = fiducial.getChild("bit18");
  check[3] = fiducial.getChild("bit24");
}

void mouseClicked() {
  render();
}

void draw() {
    background(255);
    shape(fiducial,20, 20, 440,440); 
    //video.saveFrame();
} 

void render() {
  pdf.beginDraw();
  pdf.background(255);
  pdf.textFont(font,8);
  
  int max_page_id_count = 0;
  int cur_page_id_count = 0;
  int end_id = start_id + range;
  int page = 1;
  int last_page = 0;
  
  for (int id=start_id;id<end_id;id++) {
    
    if (page>last_page) {
      println("\ncreating page #"+page);
      pdf.fill(0);
      pdf.text(license,border,pdf.height-border);
      last_page = page;
      max_page_id_count = cur_page_id_count;
      cur_page_id_count = 0;
    }
    
    if ((id<0) || (id>=1048576)) break;
    println("generating ID "+id);
    cur_page_id_count++;

    String id_bits = binary(id,20);
    int checksum = id%13;
    String check_bits = binary(checksum,4);
  
    for (int i=0;i<20;i++) {
      if (id_bits.charAt(19-i) == '1')  bits[i].setVisible(false);
      else bits[i].setVisible(true);
    }
 
    for (int i=0;i<4;i++) {
      if (check_bits.charAt(3-i) == '1')  check[i].setVisible(true);
      else check[i].setVisible(false);
    }
    
    pdf.shape(fiducial,xpos, ypos, fiducial_size,fiducial_size);
    pdf.fill(0);
    pdf.text("ID "+id,xpos,ypos+fiducial_size+border/2);

    xpos+=(border/2+fiducial_size);
    if ((xpos+border+fiducial_size)>pdf.width) {
       xpos = border;
       ypos+=(border+fiducial_size);
      
       if ((ypos+border+fiducial_size+border)>pdf.height) {
         ypos = border;
         if (id<end_id-1) {
           ((PGraphicsPDF)pdf).nextPage();
           pdf.background(255);
           page++;
         }
       }
    }
   
    redraw();
    //delay(20);
  }
  
  if (max_page_id_count!=cur_page_id_count) stencil(max_page_id_count);
  stencil(cur_page_id_count);
  
  pdf.endDraw();
  pdf.dispose();
  println("\nPDF file saved");
}

void stencil(int id_count) {
  
  ((PGraphicsPDF)pdf).nextPage();
  pdf.background(255);
  pdf.fill(0);
  xpos = border;
  ypos = border;
  pdf.fill(255);
  pdf.ellipseMode(CORNER);
  pdf.strokeWeight(0);
   
  for (int i=0;i<id_count;i++) {
    
    pdf.ellipse(xpos,ypos,fiducial_size,fiducial_size);
    xpos+=(border/2+fiducial_size);
    if ((xpos+border+fiducial_size)>pdf.width) {
       xpos = border;
       ypos+=(border+fiducial_size);
      
       if ((ypos+border+fiducial_size+border)>pdf.height) {
         ypos = border;
       }
    }
    
  } 
  
}