package dongfang.mavlink_10.messages;
public class RadioMessage extends MavlinkMessage {
  private short rssi;
  private short remrssi;
  private short txbuf;
  private short noise;
  private short remnoise;
  private int rxerrors;
  private int fixed;

  public RadioMessage(int sequence, int systemId, int componentId) {
    super(sequence, systemId, componentId);
  }

  public int getId() { return 166; }

  public String getDescription() { return "Status generated by radio"; }

  public int getExtraCRC() { return 21; }

  public int getLength() { return 9; }


  // a uint8_t
  public short getRssi() { return rssi; }
  public void setRssi(short rssi) { this.rssi=rssi; }

  // a uint8_t
  public short getRemrssi() { return remrssi; }
  public void setRemrssi(short remrssi) { this.remrssi=remrssi; }

  // a uint8_t
  public short getTxbuf() { return txbuf; }
  public void setTxbuf(short txbuf) { this.txbuf=txbuf; }

  // a uint8_t
  public short getNoise() { return noise; }
  public void setNoise(short noise) { this.noise=noise; }

  // a uint8_t
  public short getRemnoise() { return remnoise; }
  public void setRemnoise(short remnoise) { this.remnoise=remnoise; }

  // a uint16_t
  public int getRxerrors() { return rxerrors; }
  public void setRxerrors(int rxerrors) { this.rxerrors=rxerrors; }

  // a uint16_t
  public int getFixed() { return fixed; }
  public void setFixed(int fixed) { this.fixed=fixed; }

  public String toString() {
    StringBuilder result = new StringBuilder("RADIO");
    result.append(": ");
    result.append("rssi=");
    result.append(this.rssi);
    result.append(",");
    result.append("remrssi=");
    result.append(this.remrssi);
    result.append(",");
    result.append("txbuf=");
    result.append(this.txbuf);
    result.append(",");
    result.append("noise=");
    result.append(this.noise);
    result.append(",");
    result.append("remnoise=");
    result.append(this.remnoise);
    result.append(",");
    result.append("rxerrors=");
    result.append(this.rxerrors);
    result.append(",");
    result.append("fixed=");
    result.append(this.fixed);
    return result.toString();
  }
}