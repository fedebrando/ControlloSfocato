
#include <Fuzzy.h>

#define PIN_TEMP A0
#define PIN_HUM A1
#define PIN_VALVE 3

#define TOT_RULE 8

enum INPUT_TYPE
{
  TEMP,
  HUM,
  D_HUM,
  _TOT_INPUT
};

enum BLUR
{
  B_LOW,
  B_MEDIUM,
  B_HIGH,
  _TOT_BLUR
};
#define B_NOT_LOW 1

Fuzzy *ctrl;

void setup()
{
  Serial.begin(9600);

  FuzzyInput* inputs[_TOT_INPUT];
  FuzzySet* input_blurs[_TOT_INPUT][_TOT_BLUR];
  FuzzyOutput* output_valve;
  FuzzySet* output_blurs[_TOT_BLUR];

  FuzzyRuleAntecedent* ants[TOT_RULE];
  FuzzyRuleAntecedent* ant_parts[TOT_RULE];
  FuzzyRuleConsequent* cons[TOT_RULE];
  int r; // rule index

  ctrl = new Fuzzy();

  // Inputs
  input_blurs[TEMP][B_LOW] = new FuzzySet(-5, -5, 5, 10);
  input_blurs[TEMP][B_NOT_LOW] = new FuzzySet(5, 10, 60, 60);
  input_blurs[TEMP][2] = NULL;

  input_blurs[HUM][B_LOW] = new FuzzySet(0, 0, 15, 25);
  input_blurs[HUM][B_MEDIUM] = new FuzzySet(15, 25, 35, 45);
  input_blurs[HUM][B_HIGH] = new FuzzySet(35, 45, 55, 65);

  input_blurs[D_HUM][B_LOW] = new FuzzySet(-20, -15, -10, -5);
  input_blurs[D_HUM][B_MEDIUM] = new FuzzySet(-10, -5, 5, 10);
  input_blurs[D_HUM][B_HIGH] = new FuzzySet(5, 10, 15, 20);

  for (int i = 0; i < _TOT_INPUT; i++)
  {
    inputs[i] = new FuzzyInput(i);
    for (int j = 0; j < _TOT_BLUR; j++)
      if (input_blurs[i][j])
        inputs[i]->addFuzzySet(input_blurs[i][j]);
    ctrl->addFuzzyInput(inputs[i]);
  }

  // Output
  output_blurs[B_LOW] = new FuzzySet(0, 15, 30, 45);
  output_blurs[B_MEDIUM] = new FuzzySet(30, 45, 60, 75);
  output_blurs[B_HIGH] = new FuzzySet(60, 75, 100, 100);

  output_valve = new FuzzyOutput(0);
  for (int j = 0; j < _TOT_BLUR; j++)
    output_valve->addFuzzySet(output_blurs[j]);
  ctrl->addFuzzyOutput(output_valve);

  // Fuzzy rules
  for (r = 0; r < TOT_RULE; r++) // memory alloc
  {
    ants[r] = new FuzzyRuleAntecedent();
    ant_parts[r] = new FuzzyRuleAntecedent(); // you can avoid this statement for r = 0 and r = 7...
    cons[r] = new FuzzyRuleConsequent();
  }
  
  // rules
  r = 0;
  ants[r]->joinSingle(input_blurs[TEMP][B_LOW]);
  cons[r]->addOutput(output_blurs[B_LOW]);
  
  r = 1;
  ant_parts[r]->joinWithAND(input_blurs[TEMP][B_NOT_LOW], input_blurs[HUM][B_LOW]);
  ants[r]->joinWithAND(ant_parts[r], input_blurs[D_HUM][B_LOW]);
  cons[r]->addOutput(output_blurs[B_HIGH]);

  r = 2;
  ant_parts[r]->joinWithAND(input_blurs[TEMP][B_NOT_LOW], input_blurs[HUM][B_LOW]);
  ants[r]->joinWithAND(ant_parts[r], input_blurs[D_HUM][B_MEDIUM]);
  cons[r]->addOutput(output_blurs[B_MEDIUM]);

  r = 3;
  ant_parts[r]->joinWithAND(input_blurs[TEMP][B_NOT_LOW], input_blurs[HUM][B_LOW]);
  ants[r]->joinWithAND(ant_parts[r], input_blurs[D_HUM][B_HIGH]);
  cons[r]->addOutput(output_blurs[B_LOW]);

  r = 4;
  ant_parts[r]->joinWithAND(input_blurs[TEMP][B_NOT_LOW], input_blurs[HUM][B_MEDIUM]);
  ants[r]->joinWithAND(ant_parts[r], input_blurs[D_HUM][B_LOW]);
  cons[r]->addOutput(output_blurs[B_MEDIUM]);

  r = 5;
  ant_parts[r]->joinWithAND(input_blurs[TEMP][B_NOT_LOW], input_blurs[HUM][B_MEDIUM]);
  ants[r]->joinWithAND(ant_parts[r], input_blurs[D_HUM][B_MEDIUM]);
  cons[r]->addOutput(output_blurs[B_LOW]);

  r = 6;
  ant_parts[r]->joinWithAND(input_blurs[TEMP][B_NOT_LOW], input_blurs[HUM][B_MEDIUM]);
  ants[r]->joinWithAND(ant_parts[r], input_blurs[D_HUM][B_HIGH]);
  cons[r]->addOutput(output_blurs[B_LOW]);

  r = 7;
  ants[r]->joinWithAND(input_blurs[TEMP][B_NOT_LOW], input_blurs[HUM][B_HIGH]);
  cons[r]->addOutput(output_blurs[B_LOW]);

  for (r = 0; r < TOT_RULE; r++) // adding rules in the system
    ctrl->addFuzzyRule(new FuzzyRule(r, ants[r], cons[r]));

  // Pin
  pinMode(PIN_VALVE, OUTPUT);
}

float mapFloat(int val, int a1, int b1, float a2, float b2)
{
  return ((float)(val - a1) / (b1 - a1)) * (b2 - a2) + a2;
}

float speed_of(float val) // function with memory
{
  static float previous_val = 0;
  static unsigned long previous_time = 0;
  float speed;
  unsigned long time;

  time = millis();
  speed = 1000 * (val - previous_val) / (time - previous_time);
  previous_val = val;
  previous_time = time;

  return speed;
}

void loop()
{
  float input_crisps[_TOT_INPUT];
  float output_crisp;
  int output_pwm;

  // Input
  input_crisps[TEMP] = mapFloat(analogRead(PIN_TEMP), 0, 1023, -5, 60); // rescaling
  input_crisps[HUM] = mapFloat(analogRead(PIN_HUM), 0, 1023, 0, 100);
  input_crisps[D_HUM] = speed_of(input_crisps[HUM]);

  // Fuzzy control system
  for (int i = 0; i < _TOT_INPUT; i++)
    if (!ctrl->setInput(i, input_crisps[i]))
    {
      Serial.print("Error on setting input ");
      Serial.println(i);
    }
  if (!ctrl->fuzzify())
    Serial.print("Error on fuzzification");
  output_crisp = ctrl->defuzzify(0);

  Serial.print("Temperature: ");
  Serial.println(input_crisps[TEMP]);
  Serial.print("Humidity: ");
  Serial.println(input_crisps[HUM]);
  Serial.print("Humidity speed: ");
  Serial.print(input_crisps[D_HUM]);
  Serial.print(" -> Opening: ");
  Serial.println(output_crisp);

  // Output
  output_pwm = map(output_crisp, 0, 100, 0, 255);
  analogWrite(PIN_VALVE, output_pwm);

  delay(1000);
}
