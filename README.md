# Automated-Pump-Controller-Arduino
Arduino based fish pond water pump controller with fuzzy logic </br>


**required components**
- arduino uno
- water pump
- ds18b20 as temperature sensor
- mq135 as ammonia sensor
- relay
- lcd 16x4

**Fuzzy Logic Components**

Fuzzy Sets:
>These represent ranges of input and output variables. Each set has a degree of membership that indicates how much a particular value belongs to that set.

Temperature:
>Dingin (Cold): 0-20°C </br>
>Normal (Normal): 20-30°C </br>
>Panas (Hot): 30-100°C </br>

Ammonia Concentration (multiplied by 1000 to work with integers):
>Kurang (Low): 0-25 ppm </br>
>Cukup (Moderate): 25-75 ppm </br>
>Lebih (High): 75-150 ppm </br>

Pump Timer:
>Off: 0 seconds </br>
>Cepat (Fast): 30 seconds </br>
>Lama (Long): 60 seconds </br>

Fuzzy Inputs:
>Temperature and ammonia concentration are defined as fuzzy inputs.

Fuzzy Outputs:
>The output is the duration the pump should be active, represented by Timer. </br>

the code was design to easy debug with serial monitor


