# uBASIC+BACnet

uBasicPlus is an extension of the uBasic by Adam Dunkels (2006),
https://github.com/adamdunkels/ubasic, which includes
the strings and their functions from 'uBasic with strings' by David Mitchell (2008),
http://www.zenoshrdlu.com/kapstuff/zsubasic.html,
and some constructs and interpreter logic from 'CHDK',
http://chdk.wikia.com/wiki/CHDK_Scripting_Cross_Reference_Page.

uBASIC+BACnet refactored https://github.com/mkostrun/UBASIC-PLUS core data
which enables multiple uBASIC programs to run at the same time,
refactored the hardware abstraction layer using a callback pattern
which enables some or all of the hardware abstraction implementations
for any hardware platform, and added some BACnet specific keywords
for creating, reading, and writing to BACnet objects and their properties.

## uBASIC+BACnet Interpreter Features

### Software and Flow Control

- UBASIC, which can be a small non-interactive interpreter of
 the programming language BASIC which offers a subset of the commands
 that are part of the language,
 26 integer variables, named 'a' through 'z',
 *if/then/else* with support for single symbol logical operators (>,<,=),
 *for/next*, *let*, *goto*, *gosub*, *print* and '\n' (new line) as
 end-of-line character.

- uBasic with strings, which adds support for strings:
 26 string variables, named a$-z$, string literals, variables and expressions can
 be used in assignments and print statements string expressions can be compared
 for equality in if statements string expressions can be concatenated using '+',
 and a number of string functions: *left$*, *mid$*, *right*, *str$*, *chr$*,
 *val*, *len$*, *instr$*, *asc*.

- UBASIC-PLUS string scratch space (SSS) which all string variables and
 intermediate results are stored using a structure header (1byte) + data
 (strlen+1 bytes). At the end of each statement the SSS is cleared of all
 non-assigned strings. This means that rather than pointers (size 4 bytes)
 the addresses in the space are used (2 bytes). This caused all string
 functions to be rewritten. Most importantly, the garbage collection is
 done in place rather then by temporarily doubling the storage space.
 The garbage collection now used is similar to the garbage collection done
 for managing arrays.

- Elements of CHDK

  The line numbering is removed and replaced by the labels CHDK-style

  ```
  :label1
  ....
  goto label1;
  ```

  When these labels are referred to in goto/gosub this is done without
  quotation marks (unlike CHDK). Furthermore, in labels '_' can be used.
  Not even internally the lines are numbered (like it was the case with CHDK).
  Rather, pointers are used to script string, meaning that the
  returns - from gosub, for/next, if/then/else, while/endwhile - are
  faster as they do not require searches (over line numbers).

- Strings can be encompassed by single or double quotations.
  Tokenizer can identify labels.

- End of line characters *'\n'* and *';'*

- *config.h*

  Allows user to select which of the features below (if any) they want in
  their uBasic build.

- Fixep point floats are implemented through Fixed Point Math Library for C
by Ivan Voras and Tim Hartnick, https://sourceforge.net/projects/fixedptc.
The library is enhanced with str_fixedpt function, which converts a string
to fixed point float.

- flow control
  - logical operators supported (<>,<=,>=,==,&&,||,!)
  - complex logical expressions supported with use of brackets
  - multi-line If/then/else/endif-command (CHDK-style)
  - while/endwhile

- *input {a,a$,a@(i)}, timeout_ms*

   Wait at most until *timeout_ms* for input from the serial port. Wait is not
   executed inside the interpreter. Relies on external functions for
   serial-available and serial-read to be supplied as documented in *config.h*
   file.

- 26 fixed point arrays, a@ to z@, dynamically allocated using DIM command,
  ```
  dim a@(5);
  for i = 1 to 5;
    input a@(i),10000;
  next i;
  ```
  In this example code expects 5 array entries to be entered through serial
  port, and waits at most 10sec for each input to complete.
  In the scripts or from the input line the numbers can be entered either directly
  as hex values,
  ```
  ... x = 0xaabbccdd
  ```
  or as binary values,
  ```
  ... x = 0b1101001
  ```
  or as decimal values,
  ```
  ... x = 123456[d,D,L,l]
  ```
  if the default fixed point float is not desired.

- *println*

  Same as *print* but adds an empty line at the end. Additional identifiers
  *hex* or *dec* can be used to print the number as (hex)adecimal if fixed
  point floats are not desired, as in this example:
  ```
  ... println [hex,dec] x
  ```

- *sleep(f)*

  Wait *f* seconds, which can be fixed point float. This is not executed
  inside the BASIC interpreter.

- *ran, uniform*

  System random number generators based on the external function as documented
  in *config.h*. The implementation could use multiple calls to a random number
  generator to acquire the required number of random bits.

    - *ran* - generates random positive integer in fixed point float
      representation.

    - *uniform* - generates random fixed point float in the range 0 to 0.999.


- *tic(n), a=toc(n)*

  rlabplus-type timers for measuring the passed time from different
  breakpoints in the script

- *sqrt, sin, cos, tan, exp, ln, pow*

  fixed point arithmetic single argument functions from the fixed point math library.

- *floor, ceil, round, abs*

  fixed point float to fixed point integer arithmetic functions.

- *avgw*

  Returns a weighted moving average for a given a latest value, the
  previous value, and the number of samples value.

- *clear*

  Clears all variables, arrays and strings. It is good practice to put it as the
first line of the script.


### Hardware Control


- *pinmode(pin,mode,speed), a=dread(pin), dwrite(pin,state)*

  Allows direct control over digital pins. The pin, mode, speed, and state
  designations accept an 8-bit value passed to the the hardware abstraction.

- *awrite_conf(prescaler,period), awrite(channel,value), awrite(channel)*

  direct control over output pins that support analog outputs on the
  micro-controller that are passed to the hardware abstraction.
  *prescaler* and *period* are 16-bit values.
  *channel* is an 8-bit value, and *value* is a signed 32-bits integer.

- *i = flag(channel)*

  Allow flags that can be set outside BASIC interpreter,
  e.g., using interrupts, to be used in flow control.
  The *channel* is an 8-bit value, and the return is a signed 8-bit integer.
  ```
  :waithere
      if (flag(1)==0) then goto waithere;
  ```
  In this example the script sits in this loop until the hardware event
  flag no. 1 gets set by external process. Importantly, after the interpreter
  recognizes that the flag has been set, it immediately resets it so the
  subsequent calls will return 0 until the flag is set again externally.

  Importantly, while waiting for the flag, the microcontrolled does not sit
  inside the interpreter.

- *aread_conf(duration,nreads), i = aread(channel)*

  Configure and read input from analog inputs of the microcontroller.
  The *channel* is an 8-bit value, and the return is a signed 32-bit integer.

  - duration:

    8-bit sample time passed to the hardware abstraction.

  - nreads: 8-bit value passed to the hardware abstraction which can
    be used to dictate the number of sample readings to average

- *store(x[x$,x@]), i=recall(x[x$,x@])*

  Store and recall variable, string or array. In that way variables can survive
  reboot of the device.

- *bac_create(type, instance, name)*

  Create a BACnet object of *type* and *instance* where
  *type* is a number matching the BACnetObjectType enumeration
  from the BACnet Standard (0=analog input, 1=analog output, 2=analog value,
   3=binary input, 4=binary-output, 5=binary-value, etc).
  The *instance* can be any value from 0 to 4194302.
  The *name* is a string that is assigned to the object-name property of the
  BACnet object.

  ```
    bac_create(0, 1, 'ADC-1-AVG')
    bac_create(0, 2, 'ADC-2-AVG')
    bac_create(2, 1, 'ADC-1-RAW')
    bac_create(2, 2, 'ADC-2-RAW')
    bac_create(4, 1, 'LED-1')
  ```

- *i = bac_read(type, instance, property)*

  Read a numeric *property* value from the object *type* and *instance*.
  The *property* is a number matching the BACnetPropertyIdentifier
  enumeration from the BACnet standard (85=present-value).
  *type* is a number matching the BACnetObjectType enumeration
  from the BACnet Standard (0=analog input, etc).
  The *instance* can be any value from 0 to 4194302.
  The returned value is a variable type.

  ```
  a = 0
  y = bac_read(2, 1, 85)
  a = avgw(y, a, 10)
  bac_write(0, 1, 85, a)
  ```

- *bac_write(type, instance, property, value)*

  Write a variable *property* *value* to the object *type* and *instance*.
  The *property* is a number matching the BACnetPropertyIdentifier
  enumeration from the BACnet standard (85=present-value).
  *type* is a number matching the BACnetObjectType enumeration
  from the BACnet Standard (0=analog input, etc).
  The *instance* can be any value from 0 to 4194302.
  The *value* is a variable type.

  ```
  a = aread(1)
  bac_write(2, 1, 85, floor(a))
  ```

## uBASIC+BACnet SCRIPT EXAMPLES

### Demo 1 - warm up
```
println 'Demo 1 - Warm-up'
gosub l1
for i = 1 to 2
  for j = 1 to 2
    println 'i,j=',i,j
  next j
next i
println 'Demo 1 Completed'
end
  :l1
println 'subroutine'
return
```

### Demo 2 - 'uBasic with strings' by David Mitchell
```
println 'Demo 2'
a$= 'abcdefghi'
b$='123456789'
println 'Length of a$=', len(a$)
println 'Length of b$=', len(b$)
if len(a$) = len(b$) then println 'same length'
if a$ = b$ then println 'same string'
c$=left$(a$+ b$,12)
println c$
c$=right$(a$+b$, 12)
println c$
c$=mid$(a$+b$, 8,8)
println c$
c$=str$(13+42)
println c$
println len(c$)
println len('this' + 'that')
c$ = chr$(34)
println 'c$=' c$
j = asc(c$)
println 'j=' j
println val('12345')
i=instr(3, '123456789', '67')
println 'position of 67 in 123456789 is', i
println mid$(a$,2,2)+'xyx'
println 'Demo 2 Completed'
end
```

### Demo 3 - uBasic-Plus is here
```
println 'Demo 3 - Plus'
tic(1)
for i = 1 to 2
  j = i + 0.25 + 1/2
  println 'j=' j
  k = sqrt(2*j) + ln(4*i) + cos(i+j) + sin(j)
  println 'k=' k
next i
:repeat
  if toc(1)<=300 then goto repeat
for i = 1 to 2
println 'ran(' i ')=' ran
next i
for i = 1 to 2
println 'uniform(' i ')=' uniform
next i
for i = 1 to 2
x = 10 * uniform
println 'x=' x
println 'floor(x)=' floor(x)
println 'ceil(x)=' ceil(x)
println 'round(x)=' round(x)
println 'x^3=' pow(x,3)
next i
println 'Digital Write Test'
pinmode(0xc0,-1,0)
pinmode(0xc1,-1,0)
pinmode(0xc2,-1,0)
pinmode(0xc3,-1,0)
for j = 0 to 2
  dwrite(0xc0,(j % 2))
  dwrite(0xc1,(j % 2))
  dwrite(0xc2,(j % 2))
  dwrite(0xc3,(j % 2))
  sleep(0.5)
next j
println 'Press the Blue Button or type kill!'
:presswait
  if flag(1)=0 then goto presswait
tic(1)
println 'Blue Button pressed!'
:deprwait
  if flag(2)=0 then goto deprwait
println 'duration =' toc(1)
println 'Blue Button de-pressed!'
println 'Demo 3 Completed'
end
```

### Demo 4 - input array entries in 10 sec time
```
println 'Demo 4 - Input with timeouts'
dim a@(5)
for i = 1 to 5
  print '?'
  input a@(i),10000
next i
println 'end of input'
for i = 1 to 5
  println 'a(' i ') = ' a@(i)
next i
println 'Demo 4 Completed'
end
```

### Demo 5 - analog read with arrays
```
println 'Demo 5 - analog inputs and arrays';
for i = 1 to 100;
  x = aread(16);
  y = aread(17);
  println 'VREF,TEMP=', x, y;
next i;
for i = 1 to 1;
  n = floor(10 * uniform) + 2 ;
  dim b@(n);
  for j = 1 to n;
    b@(j) = ran;
    println 'b@(' j ')=' b@(j);
  next j;
next i;
println 'Demo 5 Completed';
end;
```

### Demo 6 - if/then/else/endif and while/endwhile
```
println 'Demo 6: Multiline if, while'
println 'Test If: 1'
for i=1 to 10 step 0.125
  x = uniform
  if (x>=0.5) then
    println x, 'is greater then 0.5'
  else
    println x, 'is smaller then 0.5'
  endif
  println 'i=' i
next i
println 'End of If-test 1'
println 'Test While: 1'
i=10
while ((i>=0)&&(uniform<=0.9))
  i = i - 0.125
  println 'i =', i
endwhile
println 'End of While-test 1'
println 'Demo 6 Completed'
end
```

### Demo 7 - Analog Read Forever Loop
```
println 'Demo 7: Analog Read Forever Loop'
y=0
:startover
  x = aread(10)
  if (abs(x-y)>20) then
    y = x
    println 'x=',x
  endif
  sleep (0.2)
  goto startover
end
```


### Demo 8 - Analog Output Test
```
println 'Demo 8: analog write 4-Channel Test'
p = 65536
for k = 1 to 10
  p = p/2
  awrite_conf(p,4096)
  println 'prescaler = ' p
  for i = 1 to 10
    for j = 1 to 4
      awrite(j,4095*uniform)
    next j
    println '    analog write = ' awrite(1),awrite(2),awrite(3),awrite(4)
    sleep(5)
  next i
next k
awrite(1,0)
awrite(2,0)
awrite(3,0)
awrite(4,0)
end
```

### Demo 9 - Store/recall in non-volatile memory Test
```
clear
println 'Demo 9: store/recall to non-volatile memory'
if (recall(x)==0) then
  println 'generating x'
  x = uniform
  store(x)
endif
println 'stored: x=' x
if (recall(y@)==0) then
  println 'generating y'
  dim y@(10)
  for i=1 to 10
    y@(i) = uniform
  next i
  store(y@)
endif
println 'stored: y@'
for i=1 to 10
  println '  y@('i')=' y@(i)
next i
if (recall(s$)==0) then
  println 'generating s'
  s$='what is going on?'
  store(s$)
endif
println 'stored: s$',s$
println 'Demo 9 Completed'
println 'Please run it once more'
end
```

### Demo 10 - BACnet & GPIO
```
println 'Demo - BACnet & GPIO'
bac_create(0, 1, 'ADC-1-AVG')
bac_create(0, 2, 'ADC-2-AVG')
bac_create(2, 1, 'ADC-1-RAW')
bac_create(2, 2, 'ADC-2-RAW')
bac_create(4, 1, 'LED-1')
bac_create(2, 3, 'Counter 1')
bac_create(2, 4, 'Counter 2')
bac_create(2, 5, 'Counter 1-FLOOR')
bac_create(2, 6, 'Counter 1-CEIL')
bac_create(2, 7, 'Counter 1-ROUND')
bac_create(2, 8, 'Counter 1-POW3')
y = 0.1
z = 0.0
bac_write(2, 3, 85, y)
bac_write(2, 4, 85, z)
:startover
  y = bac_read(2, 3, 85)
  y = y + 1.1
  if (y > 100.0) then
    y = 0.0
    z = z + 1.0
  endif
  if (z > 1000.0) then
    z = 0.0
  endif
  bac_write(2, 3, 85, y)
  bac_write(2, 4, 85, z)
  bac_write(2, 5, 85, floor(y))
  bac_write(2, 6, 85, ceil(y))
  bac_write(2, 7, 85, round(y))
  bac_write(2, 8, 85, pow(y, 3))
  a = aread(1)
  bac_write(2, 1, 85, a)
  c = avgw(a, c, 10)
  bac_write(0, 1, 85, c)
  b = aread(2)
  bac_write(2, 1, 85, b)
  d = avgw(b, d, 10)
  bac_write(0, 2, 85, d)
  h = bac_read(4, 1, 85)
  dwrite(1, (h % 2))
  sleep (0.2)
goto startover
end
```

## uBASIC+BACnet HARDWARE ABSTRACTION LAYER

The uBASIC+BACnet hardware abstraction layer is integrated using
a set of callback functions assigned to each `struct ubasic_data`
defined in *ubasic/ubasic.h* file. The structure members depend
on *ubasic/config.h* file defines so that the abstraction can be
as small or large as the implementation allows or requires.

```
    /* API for hardware drivers */
    #if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
        void (*pwm_config)(uint16_t psc, uint16_t per);
        void (*pwm_write)(uint8_t ch, int32_t dutycycle);
        int32_t (*pwm_read)(uint8_t ch);
    #endif
    #if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
        void (*gpio_config)(uint8_t ch, int8_t mode, uint8_t freq);
        void (*gpio_write)(uint8_t ch, uint8_t pin_state);
        int32_t (*gpio_read)(uint8_t ch);
    #endif
    #if (                                              \
        defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) || \
        defined(UBASIC_SCRIPT_HAVE_SLEEP) ||           \
        defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL))
        uint32_t (*mstimer_now)(void);
    #endif
    #if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
        void (*adc_config)(uint8_t sampletime, uint8_t nreads);
        int32_t (*adc_read)(uint8_t channel);
    #endif
    #if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
        int8_t (*hw_event)(uint8_t bit);
        void (*hw_event_clear)(uint8_t bit);
    #endif
    #if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
        uint32_t (*random_uint32)(uint8_t size);
    #endif
    #if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
        void (*variable_write)(
            uint8_t Name, uint8_t Vartype, uint8_t datalen_bytes, uint8_t *dataptr);
        void (*variable_read)(
            uint8_t Name, uint8_t Vartype, uint8_t *dataptr, uint8_t *datalen);
    #endif
    #if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
        int (*ubasic_getc)(void);
    #endif
    #if defined(UBASIC_SCRIPT_HAVE_PRINT_TO_SERIAL)
        void (*serial_write)(const char *buffer, uint16_t n);
    #endif
    #if defined(UBASIC_SCRIPT_HAVE_BACNET)
        void (*bacnet_create_object)(
            uint16_t object_type, uint32_t instance, char *object_name);
        void (*bacnet_write_property)(
            uint16_t object_type,
            uint32_t instance,
            uint32_t property_id,
            UBASIC_VARIABLE_TYPE value);
        UBASIC_VARIABLE_TYPE(*bacnet_read_property)
        (uint16_t object_type, uint32_t instance, uint32_t property_id);
    #endif
```

## uBASIC+BACnet APPLICATION INTEGRATION

The uBASIC+BACnet comprise of six files config.h, fixedptc.h, tokenizer.c,
tokenizer.h, ubasic.c and ubasic.h.

An example implementation of the hardware related functions (random
number generation, gpio, hardware events, sleep and tic/toc) and usage
of the applicatlion layer functions can be found in the `ports/stm32f4xx`
project.

- `ports/stm32f4xx/program-ubasic.c`

  A BACnet Program object that handles Load, Start, Run, Halt, Restart,
  and Unload values from the BACnetProgramRequest enumeration options
  integrated with the uBASIC+BACnet API:
  - `ubasic_port_init()`
  - `ubasic_clear_variables()`
  - `ubasic_load_program()`
  - `ubasic_halt_program()`
  - `ubasic_run_program()`

  Additionally the Program object provides a glimpse inside the
  uBASIC+BACnet script using `ubasic_program_location()` function.

- `ports/stm32f4xx/ubasic-port.c`

  A port of the hardware layer to STM32F4xx board and the BACnet API functions

- `ports/stm32f4xx/main.c`

  The hardware setup and example uBASIC+BACnet programs running on the
  hardware as BACnet File objects used by BACnet Program objects.
