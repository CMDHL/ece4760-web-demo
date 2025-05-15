TARGET = game
SRC = animation.c
PORT = 4760

all: build

build:
	emcc animation.c -o game.js \
		-sEXPORTED_FUNCTIONS=_main,_ui_state_machine \
		-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,HEAPU32 \
		-sALLOW_MEMORY_GROWTH=1 \
		-sASYNCIFY

run: build
	@echo "Killing any server on port $(PORT)..."
	@lsof -ti tcp:$(PORT) | xargs -r kill -9
	@echo "Opening browser..."
	@open http://localhost:$(PORT) &
	@echo "Starting server at http://localhost:$(PORT) (Ctrl+C to stop)"
	@python3 -m http.server $(PORT)

clean:
	rm -f $(TARGET).js $(TARGET).wasm
