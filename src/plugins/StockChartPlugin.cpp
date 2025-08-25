#include "plugins/StockChartPlugin.h"
#include "screen.h"

static void drawChart(){
  const StockData &d = StockService::getInstance().get();
  Screen.beginUpdate();
  Screen.clear();
  if (!d.valid || d.count==0){
    // simple loading shimmer
    for (int i=0;i<16;i++) Screen.setPixel(i,15-(i%3),1,80);
    Screen.endUpdate();
    return;
  }
  // find min/max; guard against outliers by using last N only
  int n = d.count; if (n>16) n=16;
  int startIdx = d.count - n;
  float mn = d.close[startIdx], mx = d.close[startIdx];
  for (int i=startIdx+1;i<startIdx+n;i++){ if (d.close[i]<mn) mn=d.close[i]; if (d.close[i]>mx) mx=d.close[i]; }
  if (mx==mn) mx=mn+1.0f;
  // map to 16x16: use last up to 16 points across width
  // draw grid (faint)
  for (int y=0;y<16;y+=4) for (int x=0;x<16;x++) Screen.setPixel(x,y,1,20);
  for (int x=0;x<16;x+=4) for (int y=0;y<16;y++) Screen.setPixel(x,y,1,20);

  int m = n; // points to draw
  int prevx=-1, prevy=-1;
  for (int i=0;i<m;i++){
    float v = d.close[startIdx + i];
    int x = (16 - m) + i; if (x<0) x=0; // right-align
    float t = (v - mn)/(mx - mn);
    int y = 15 - (int)roundf(t * 15.0f);
    if (x>=0 && x<16 && y>=0 && y<16){
      Screen.setPixel(x,y,1,220);
      if (prevx>=0){
        // simple vertical connect to show trend
        int y0=min(prevy,y), y1=max(prevy,y);
        for (int yy=y0; yy<=y1; ++yy) Screen.setPixel(x,yy,1,120);
      }
      prevx=x; prevy=y;
    }
  }
  // baseline
  for (int x=0;x<16;x++) Screen.setPixel(x,15,1,30);
  Screen.endUpdate();
}

void StockChartPlugin::loop(){
  static unsigned long nextTick = 0;
  unsigned long now = millis();
  if (now >= nextTick){
    drawChart();
    nextTick = now + 500; // 2 Hz refresh
  }
}

