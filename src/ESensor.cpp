#include <ESensors.h>
#include <ESensor.h>


ESensor::ESensor() :
  ESensorDevice(),
  Name(""),
  Symbol(""),
  BasicUnit(""),
  Unit(""),
  Format("%.5g"),
  CompactFormat("%.5g"),
  Factor(1.0),
  Offset(0.0),
  Resolution(1.0) {
}


ESensor::ESensor(const char *name, const char *symbol, const char *unit,
		 const char *format, float resolution) :
  ESensor() {
  strcpy(BasicUnit, unit);
  setName(name);
  setSymbol(symbol);
  setUnit(unit, 1.0, 0.0);
  setFormat(format);
  setResolution(resolution);
}


ESensor::ESensor(ESensors *sensors, const char *name, const char *symbol,
		 const char *unit, const char *format, float resolution) :
  ESensor(name, symbol, unit, format, resolution) {
  if (sensors != 0)
    sensors->addSensor(*this);
}


ESensorDevice *ESensor::device() {
  return this;
}


const char* ESensor::name() const {
  return Name;
}


void ESensor::setName(const char *name) {
  strcpy(Name, name);
}


void ESensor::setName(const char *name, const char *symbol) {
  strcpy(Name, name);
  strcpy(Symbol, symbol);
}


const char* ESensor::symbol() const {
  return Symbol;
}


void ESensor::setSymbol(const char *symbol) {
  strcpy(Symbol, symbol);
}


const char* ESensor::basicUnit() const {
  return BasicUnit;
}


const char* ESensor::unit() const {
  return Unit;
}


void ESensor::setUnit(const char *unit, float factor, float offset) {
  strcpy(Unit, unit);
  Factor = factor;
  Offset = offset;
}


void ESensor::setUnit(const char *unit, float factor, float offset,
		      const char *format) {
  strcpy(Unit, unit);
  Factor = factor;
  Offset = offset;
  setFormat(format);
}


void ESensor::setUnit(const char *unit, float factor, const char *format) {
  strcpy(Unit, unit);
  Factor = factor;
  Offset = 0.0;
  setFormat(format);
}


const char* ESensor::format() const {
  return Format;
}


const char* ESensor::compactFormat() const {
  return CompactFormat;
}


void ESensor::setFormat(const char *format) {
  strcpy(Format, format);
  // format without width:
  char fpref[4];
  int width;
  int decis;
  char ftype[4];
  parseFormat(fpref, &width, &decis, ftype);
  if (decis >= 0)
    sprintf(CompactFormat, "%%%s.%d%s", fpref, decis, ftype);
  else
    sprintf(CompactFormat, "%%%s%s", fpref, ftype);
}


void ESensor::setResolution(float resolution) {
  Resolution = resolution;
}


float ESensor::resolution() const {
  return Factor*Resolution;
}


int ESensor::resolutionStr(char *s, bool compact) const {
  if (compact)
    return sprintf(s, CompactFormat, resolution());
  else
    return sprintf(s, Format, resolution());
}


int utf8_strlen(const char *str) {
  // http://www.zedwood.com/article/cpp-utf8-strlen-function
  unsigned int i, n;
  for (n=0, i=0; i<strlen(str); i++, n++) {
    char c = str[i];
    if      (c >= 0   && c <= 127) i += 0;
    else if ((c & 0xE0) == 0xC0) i += 1;
    else if ((c & 0xF0) == 0xE0) i += 2;
    else if ((c & 0xF8) == 0xF0) i += 3;
    else return 0; //invalid utf8
  }
  return n;
}


void ESensor::report(Stream &stream) {
  if (available()) {
    char rs[10];
    resolutionStr(rs, true);
    stream.printf("%-20s %-8s", name(), symbol());
    int nunit = 6 - utf8_strlen(unit());
    if (strlen(unit()) > 0)
      stream.printf(" (%s)", unit());
    else
      stream.print("   ");
    if (nunit > 0)
      stream.printf("%-*s", nunit, "");
    stream.printf(" at a resolution of %5s%s", rs, unit());
    if (nunit > 0)
      stream.printf("%-*s", nunit, "");
    if (strlen(chip()) > 0)
      stream.printf(" on %-12s device", chip());
    else
      stream.printf("%23s", "");
    if (strlen(identifier()) > 0)
      stream.printf(" with ID %s", identifier());
    stream.println();
  }
}


float ESensor::value() const {
  return Factor*reading() + Offset;
}


int ESensor::valueStr(char *s, bool compact) const {
  if (compact)
    return sprintf(s, CompactFormat, value());
  else
    return sprintf(s, Format, value());
}


float ESensor::read() {
  ESensorDevice::read();
  return value();
}


void ESensor::parseFormat(char *prefix, int *width,
			  int *decimals, char *ftype) const {
  *prefix = '\0';
  *width = -1;
  *decimals = -1;
  *ftype = '\0';
  const char *sp = Format;
  // percent:
  if (*sp != '%')
    return;
  ++sp;
  // prefix:
  while (*sp == '0' || *sp == '+' || *sp == '-')
    *(prefix++) = *(sp++);
  *prefix = '\0';
  // width:
  char ws[10];
  char *wp = ws;
  while (isdigit(*sp))
    *(wp++) = *(sp++);
  *wp = '\0';
  if (strlen(ws) > 0)
    *width = atoi(ws);
  // decimals:
  if (*sp == '.') {
    ++sp;
    char *wp = ws;
    while (isdigit(*sp))
      *(wp++) = *(sp++);
    *wp = '\0';
    if (strlen(ws) > 0)
      *decimals = atoi(ws);
  }
  // format type:
  while (*sp != '\0')
    *(ftype++) = *(sp++);
  *ftype = '\0';
}


void ESensor::adaptFormat(int decimals) {
  // parse format string:
  char fpref[4];
  int width;
  int decis;
  char ftype[4];
  parseFormat(fpref, &width, &decis, ftype);
  // adapt format:
  if (strcmp(ftype, "f") == 0) {
    int org_decis = decis;
    if (decis >= 0) {
      decis += decimals;
      if (decis < 0)
	decis = 0;
    }
    if (width >= 0) {
      if (org_decis > 0 && decis == 0)
	width--;
      else if (org_decis == 0 && decis > 0)
	width++;
    }
  }
  // assemble format string:
  char format[22];
  if (width >= 0 && decis >= 0)
    sprintf(format, "%%%s%d.%d%s", fpref, width, decis, ftype);
  else if (width >= 0)
    sprintf(format, "%%%s%d%s", fpref, width, ftype);
  else if (decis >= 0)
    sprintf(format, "%%%s.%d%s", fpref, decis, ftype);
  else
    sprintf(format, "%%%s%s", fpref, ftype);
  setFormat(format);
}


void ESensor::setSIPrefix(const char *prefix, float factor, int decimals) {
  // new unit string:
  char us[20];
  sprintf(us, "%s%s", prefix, unit());
  // set unit:
  setUnit(us, factor);
  adaptFormat(decimals);
}


void ESensor::setPercent() {
  setUnit("%", 100.0);
  adaptFormat(-2);
}


void ESensor::setKelvin() {
  setUnit("K", 1.0, 273.15);
}


void ESensor::setFahrenheit() {
  setUnit("F", 9.0/5.0, 32.0);
}


void ESensor::setBar() {
  setUnit("bar", 1e-5);
  adaptFormat(5);
}


void ESensor::setMilliBar() {
  setUnit("mbar", 0.01);
  adaptFormat(2);
}


void ESensor::setAt() {
  setUnit("at", 0.0000101971621298);
  adaptFormat(5);
}


void ESensor::setAtm() {
  setUnit("atm", 0.00000986923266716);
  adaptFormat(5);
}


void ESensor::setMMHg() {
  setUnit("mmHg", 0.00750061575846);
  adaptFormat(2);
}


void ESensor::setPSI() {
  setUnit("psi", 0.00014503773773);
  adaptFormat(4);
}


void ESensor::setTorr() {
  setUnit("torr", 0.00750061682704);
  adaptFormat(2);
}

