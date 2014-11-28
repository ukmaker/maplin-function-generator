
final int steps = 16;
final int bits = 4;
final int W = 400;
final int H = 400;
final int A = H/2;
final int YOFFSET = 10;
final int maxVal = (1 << (bits - 1)) - 1;

void setup() {
  
  size(W+2*YOFFSET,H+2*YOFFSET);
  // draw axes
  // x-axis
  line(0, A+YOFFSET, W+2*YOFFSET, A+YOFFSET);
  for(int i=0; i<=steps; i++) {
    // tick marks
    line(i*H/steps,A+YOFFSET+5,i*H/steps,A+YOFFSET-5);
  }
  
  // y-axis
  int m = 1<<(bits-1);
  for(int i=-m; i<=m; i++) {
    int y = i*A/m + A + YOFFSET;
    line(0,y,5,y);
  }


  int x=0;
  int y=A;
  for(int i=0; i <= steps; i++) {
    int sine = quantizeAndScale(sin((float)(2*Math.PI*i/steps)));
    int s = A * sine / maxVal;
    line(x,y+YOFFSET,i*H/steps,y+YOFFSET);
    line(i*H/steps,y+YOFFSET,i*H/steps,A-s+YOFFSET);
    x=i*H/steps;
    y=A-s;
    
    println(sine+" ");
  }

}

int quantizeAndScale(float s) {
  // Convert s to a number with the correct number of bits, and scale to A
  int sb = Math.round(s * maxVal);
  
  return sb;
}
