/**
* Demonstrate the effect of sampling rate on the output signal
**/
final int tableSize = 16;
final int bits = 4;
final int W = 800;
final int H = 400;
final int A = H/2;
final int maxVal = (1 << (bits - 1)) - 1;
final int XSCALE = 10;
final int XTICKS = W / XSCALE;

int[] table = new int[tableSize];
int lastVal = 0;

/**
* DDS parameters
**/
long phaseAcc;
long phaseDelta;

void setup() {
  createTable();
  createGraph();
  plotSample();
}

void createTable() {
  int x=0;
  int y=A;
  for(int i=0; i < tableSize; i++) {
    int sine = quantizeAndScale(sin((float)(2*Math.PI*i/tableSize)));
    table[i] = sine;
  }
}

int quantizeAndScale(float s) {
  // Convert s to a number with the correct number of bits, and scale to A
  int sb = Math.round(s * maxVal);
  
  return sb;
}

void createGraph() {
    size(W,H);
  // draw axes
  // x-axis
  line(0, A, W, A);
  for(int i=0; i<=XTICKS; i++) {
    // tick marks
    line(i*XSCALE,A+5,i*XSCALE,A-5);
  }
  
  // y-axis
  int m = 1<<(bits-1);
  for(int i=-m; i<=m; i++) {
    int y = i*A/m + A;
    line(0,y,5,y);
  }
}

void plotSample() {
  phaseAcc = 0L;
  phaseDelta = 1024L;
  for(int i=0; i< XTICKS; i++) {
    plot(i);
    phaseAcc += phaseDelta;
  }
}

void plot(int t) {
  int y = y(lookup());
  
  if(t != 0) {
    line(x(t-1),lastVal,x(t),y);
  }
  
  lastVal = y;
}

int x(int x) {
  return x*XSCALE;
}

int y(int y) {
  return A - y*A/maxVal;
}

// Lookup the current value pointed to by the phaseAddress
int lookup() {
  return table[(int)(phaseAcc & 0xff00) >> 12];
}

