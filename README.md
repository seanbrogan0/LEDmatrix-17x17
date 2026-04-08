<!-- ============================= -->
<!-- Project Status Notice         -->
<!-- ============================= -->

<div style="border: 2px solid #f0ad4e; padding: 1em; margin-bottom: 1.5em; background-color: #fff8e1;">
  <p style="margin: 0; font-size: 1.1em;">
    🚧 <strong>Work in Progress</strong>
  </p>
  <p style="margin: 0.5em 0 0 0;">
    This repository is actively under development. Currently in very early stage. Core architecture is in place,
    but additional effects, refinements, and documentation are still evolving. Expect failures and bugs everywhere...
  </p>
</div>

<h1>LEDmatrix‑17x17 – Serpentine LED Matrix Firmware</h1>

<p>
A modular, PlatformIO‑based firmware project for a
<strong>17×17 serpentine‑wired LED matrix</strong>, designed to support
multiple standalone animations such as seasonal displays, ambient effects,
and physics‑based motion.
</p>

<p>
This project prioritises <strong>clean firmware architecture</strong>,
<strong>hardware abstraction</strong>, and
<strong>long‑term maintainability</strong> over one‑off Arduino sketches.
</p>

<hr />

<h2>Project Overview</h2>

<p>
The hardware consists of a 17×17 grid of addressable RGB LEDs wired in a
serpentine pattern. The firmware provides a clean XY coordinate abstraction
so that all animations can be written in logical 2D space, independent of
physical wiring order.
</p>

<p>
Rather than maintaining multiple disconnected sketches, this repository
represents a <strong>single hardware topology</strong> with
<strong>multiple effects</strong> that can be enabled or extended over time.
</p>

<hr />

<h2>Hardware Summary</h2>

<ul>
  <li><strong>Microcontroller:</strong> Arduino Nano (ATmega328P)</li>
  <li><strong>LEDs:</strong> Addressable RGB LEDs (WS2812 / NeoPixel compatible)</li>
  <li><strong>Matrix Size:</strong> 17 × 17 (289 LEDs)</li>
  <li><strong>Wiring:</strong> Serpentine (alternating row direction)</li>
  <li><strong>Inputs:</strong> Potentiometers and buttons (effect‑dependent)</li>
  <li><strong>Enclosure:</strong> Custom 3D‑printed frame and diffuser</li>
</ul>

<hr />

<h2>Firmware Architecture</h2>

<p>
This project deliberately avoids a monolithic Arduino sketch.
Instead, the firmware is divided into clearly defined modules,
each with a single responsibility.
</p>

<pre>
src/
├── main.cpp                  # Application entry point & orchestration
├── matrix.cpp                # XY → LED index mapping (serpentine wiring)
├── input.cpp                 # Potentiometer & button handling
├── effects/
│   └── bouncing_ball.cpp     # Example animation effect
│
include/
├── config.h                  # Matrix size, pins, compile‑time configuration
├── matrix.h                  # Matrix mapping interface
├── input.h                   # Input abstraction interface
├── effects.h                 # Effect entry points
├── globals.h                 # Shared LED buffer (single definition)
</pre>

<h3>Design Goals</h3>

<ul>
  <li>
    <strong>Explicit ownership</strong><br />
    Hardware resources and shared state are defined exactly once and accessed
    via clear interfaces.
  </li>
  <li>
    <strong>Separation of concerns</strong><br />
    Geometry, input handling, animation logic, and rendering are not interleaved.
  </li>
  <li>
    <strong>Scalability</strong><br />
    New effects can be added without modifying core infrastructure.
  </li>
  <li>
    <strong>Deterministic behaviour</strong><br />
    No blocking delays; timing is based on <code>millis()</code>.
  </li>
</ul>

<hr />

<h2>Key Modules Explained</h2>

<h3><code>main.cpp</code></h3>
<p>
High‑level orchestration only:
</p>
<ul>
  <li>Initialises FastLED and hardware</li>
  <li>Initialises input handling</li>
  <li>Calls the active effect once per loop</li>
  <li>Commits LED buffer to hardware</li>
</ul>

<h3><code>matrix.cpp</code></h3>
<p>
Defines the physical layout of the LED matrix:
</p>
<ul>
  <li>Maps logical (x, y) coordinates to LED indices</li>
  <li>Encapsulates serpentine wiring behaviour</li>
  <li>Allows animations to be written in 2D space</li>
</ul>

<h3><code>input.cpp</code></h3>
<p>
Handles all physical user inputs:
</p>
<ul>
  <li>Potentiometer smoothing</li>
  <li>Brightness and speed mapping</li>
  <li>Future expansion for buttons or modes</li>
</ul>

<h3><code>effects/</code></h3>
<p>
Contains individual animation implementations:
</p>
<ul>
  <li>Each effect is self‑contained</li>
  <li>No direct hardware access</li>
  <li>Uses matrix and input abstractions only</li>
</ul>

<h3><code>config.h</code></h3>
<p>
Single source of truth for:
</p>
<ul>
  <li>Matrix dimensions</li>
  <li>LED count</li>
  <li>Pin assignments</li>
  <li>Default tuning values</li>
</ul>

<hr />

<h2>Development Environment</h2>

<ul>
  <li><strong>IDE:</strong> Visual Studio Code</li>
  <li><strong>Build System:</strong> PlatformIO</li>
  <li><strong>Framework:</strong> Arduino (AVR)</li>
  <li><strong>Key Library:</strong> FastLED</li>
</ul>

<p>
This project is intentionally <strong>PlatformIO‑native</strong> and is not
designed to be maintained using the Arduino IDE.
</p>

<hr />

<h2>CAD & Electronics</h2>

<p>
This repository also contains supporting assets:
</p>

<ul>
  <li><strong>3D Models:</strong> Custom frame, grid, diffuser, and legs</li>
  <li><strong>Circuit:</strong> Fritzing schematic for matrix wiring</li>
</ul>

<p>
These assets are designed alongside the firmware to ensure physical layout,
diffusion, and wiring match the assumptions in code.
</p>

<hr />

<h2>Project Status</h2>

<ul>
  <li>✅ Matrix abstraction complete</li>
  <li>✅ Input handling extracted</li>
  <li>✅ First effect (Bouncing Ball) ported</li>
  <li>🚧 Additional effects in progress</li>
</ul>

<p>
Planned future work includes:
</p>

<ul>
  <li>Additional seasonal and ambient effects</li>
  <li>Build‑time or runtime effect selection</li>
  <li>Button‑driven colour and mode changes</li>
  <li>Performance and memory optimisation</li>
</ul>

<hr />

<h2>License</h2>

<p>
This project is licensed under the
<strong>Creative Commons Attribution–NonCommercial 4.0</strong> license.
</p>

<p>
You are free to use and modify this project for
<strong>non‑commercial purposes</strong> only.
Commercial use requires explicit permission from the author.
See <code>LICENSE</code> for full details.
</p>
