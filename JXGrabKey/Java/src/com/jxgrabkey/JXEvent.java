package com.jxgrabkey;

public class JXEvent {
  public String display;
  public int screen;
  public java.awt.Point point;

  public JXEvent(String display, int screen, int x, int y) {
    this.display = display;
    this.screen = screen;
    this.point = new java.awt.Point(x, y);
  }
}
