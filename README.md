# GLed - A led control class

## Description

The GLed class models an LED. It provides methods to manipulate the LED
and switch it on and off. This conceals the fact that the switching logic of the
LED can be positive or negative depending on the type of electronic circuit.
Some LEDs are switched on by switching the pin that controls the LED to HIGH (3.3V).
Others light up by switching the control pin to LOW (GND). By abstracting this detail,
code can be created whose on/off commands are identical for all boards.
To do this, a GLed object only needs to be told which pin is used and whether
the switching logic is positive (HIGH) or negative (GND) when it is created.

## Installation

just copy the GLed top directory into your active Arduino libraries directory.

## Examples

  There are some  examples implemented in this library. 
  Please check the Arduino menu for GLed examples.
  
## Contributing

If you want to contribute to this project:

- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell others about this library
- Contribute new protocols


- ### Give back

    The library is free; you don’t have to pay for anything. However, if you want to support the development contect the author.

## Credits

The author and maintainer of this library are <gruenkohl089@gmail.com>.

## License

This library is licensed under [MIT license](https://opensource.org/licenses/MIT). All text above must be included in any redistribution.
