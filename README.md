# Dangerous Forest - Multi-Resolution Demo

A first-person demo built with raylib showcasing multi-resolution rendering with proper compositing and occlusion. This demo highlights the technique of rendering props at lower resolution than the environment for performance optimization while maintaining visual quality.

## Features
- First-person camera movement in a 3D environment
- Multiple interconnected rooms with props
- Props rendered at 1/4 the quality of the game's 720p resolution
- Proper occlusion of low-resolution props against high-resolution environment
- Toggle between high and low resolution props
- Automatic toggling between high and low resolution for easy comparison
- Visually distinct textures to highlight resolution differences
- Modular architecture for organization, performance, and scalability

## Controls
- W, A, S, D: Move around the environment
- Mouse: Look around
- H: Manually toggle between high and low resolution props
- I: Toggle debug information display
- ESC: Exit demo

## Building and Running
```bash
# Install dependencies
sudo apt-get install build-essential git libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

# Build the project
make

# Run the game
./dangerous_forest
```

## Project Structure
- `src/`: Source code
  - `core/`: Core game systems
  - `renderer/`: Rendering systems
  - `world/`: World, rooms, and props
  - `utils/`: Utility functions
- `assets/`: Game assets
  - `textures/`: Texture files
  - `models/`: 3D model files
