import processing.pdf.*;

PFont font;
PShape fiducial;
PShape bits[] = new PShape[20];
PShape check[] = new PShape[4];

PGraphics pdf;

int border = 14;
int fiducial_size_mm = 36;
int fiducial_size;

int xpos = border;
int ypos = border;

int start_id = 0;
int range = 300;
 
void setup() {
  pdf = createGraphics(598, 842, PDF, "yamaarashi_"+start_id+"-"+(start_id+range-1)+".pdf");
  font = createFont("Arial", 8);
  
  /*int logo = unbinary("10111000010010110010");
  int cs_logo = logo%16;
  String cs_bin = binary(cs_logo,4);
  println(logo+" "+cs_bin);*/
  
  fiducial_size = round(fiducial_size_mm * 2.83464567);
  
  fiducial = loadShape("yamaarashi.svg");
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
  
  check[0] = fiducial.getChild("bit6");
  check[1] = fiducial.getChild("bit12");
  check[2] = fiducial.getChild("bit18");
  check[3] = fiducial.getChild("bit24");

  pdf.beginDraw();
  render();
  pdf.dispose();
  pdf.endDraw();
  exit();
}


void render() {
  pdf.background(255);
  pdf.textFont(font,8); 
  
  int end_id = start_id + range;
  int page = 1;
  
  println("creating page #"+page);
  for (int id=start_id;id<end_id;id++) {
    if ((id<0) || (id>=1048576)) break;
    println("generating ID "+id);
  
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
    pdf.text("ID "+id,xpos,ypos+fiducial_size+border);
      
    xpos+=(border+fiducial_size);
    if ((xpos+border+fiducial_size)>pdf.width) {
       xpos = border;
       ypos+=(2*border+fiducial_size);
      
       if ((ypos+border+fiducial_size+border)>pdf.height) {
         ypos = border;
         if (id<end_id-1) {
           pdf.text("reacTIVision yamaarashi symbols CC BY-NC-SA 2016 Martin Kaltenbrunner <martin@tuio.org>",border,pdf.height-border);
           page++;
           println("\ncreating page #"+page);
           ((PGraphicsPDF)pdf).nextPage();
           pdf.background(255);
         }
       } 
    }
    
  }
  
  pdf.text("reacTIVision yamaarashi symbols CC BY-NC-SA 2016 Martin Kaltenbrunner <martin@tuio.org>",border,pdf.height-border);
  println("\nPDF file saved");
}