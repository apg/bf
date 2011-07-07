Function.prototype.bind = function(scope) {
  var _function = this;

  return function() {
    return _function.apply(scope, arguments);
  }
};

BrainFuck = function(inp, out) {
	this._pc = 0;
	this._lpc = 0;
	this._heap = [];
	this._heapsize = 1024;
	this._dp = 0;
	this._opens = 0;
	this._loopstack = [];
	this.getinput = inp;
	this.output = out;

	this._stepper = this.step.bind(this);

	for (var i = 0; i < this._heapsize; i++) {
		this._heap[i] = 0;
	}

};

BrainFuck.prototype = {
	reset: function() {
		this._lpc = 0;
		this._pc = 0;
		this._heap = [];
		this._heapsize = 1024;
		this._dp = 0;
		this._opens = 0;
		this._loopstack = [];
		for (var i = 0; i < this._heapsize; i++) {
			this._heap[i] = 0;
		}
		if (this.onreset) {
			this.onreset(this);
		}
	},
	currentInstruction: function() {
		return this._code[this._pc];
	},
	currentPC: function() {
		return this._pc;
	},
	lastPC: function() {
		return this._lpc;
	},
	currentDP: function() {
		return this._dp;
	},
	currentAtDP: function() {
		return this._heap[this._dp];
	},
	code: function() {
		return this._code;
	},
	heap: function() {
		return this._heap;
	},
	exec: function(code) {
		this._code = code;
		if (this.onpreexec) {
			this.onpreexec(this);
		}
		setTimeout(this._stepper, 0);
	},
	step: function() {
		if (this._pc < this._code.length) {
			if (this.onpreinst) {
				this.onpreinst(this);
			}

			var inst = this._code[this._pc];
			if (inst === '>') {
				var ndp = this._dp + 1;
				if (ndp < this._heapsize) {
					this._dp = ndp;
				}
				else {
					if (this.onerror) {
						this.onerror(this, "Heap out of bounds.");
						return -1;
					}
				}
			}
			else if (inst === '<') {
				var ndp = this._dp - 1;
				if (ndp >= 0) {
					this._dp = ndp;
				}
				else {
					if (this.onerror) {
						this.onerror(this, "Heap out of bounds.");
						return -1;
					}
				}
			}
			else if (inst === '+') {
				if (this._heap[this._dp] === undefined) {
					this._heap[this._dp] = 0;
				}
				this._heap[this._dp] += 1;
			}
			else if (inst === '-') {
				if (this._heap[this._dp] === undefined) {
					this._heap[this._dp] = 0;
				}
				this._heap[this._dp] -= 1;
			}
			else if (inst === '.') {
				this.output(this._heap[this._dp]);
			}
			else if (inst === ',') {
				this._heap[this._dp] = this.getinput();
			}
			else if (inst === '[') {
				if (this._heap[this._dp] == 0) {
					var newpc = this._find_next_jump_point(this._pc);
					if (newpc < 0) {
						if (this.onerror) {
							this.onerror(this, "No end loop instruction found after loop open.");
							return -1;
						}
					}
				}
				else {
					this._loopstack.push(this._pc);
				}
			}
			else if (inst === ']') {
				if (this._loopstack.length > 0) {
					if (this._heap[this._dp] != 0) {
						this._lpc = this._pc;
						this._pc = this._loopstack.pop();
						if (this.onpostinst) {
							this.onpostinst(this);
						}
						setTimeout(this._stepper, 0);
						return;
//						continue; // skip the increment
					}
				}
				else {
					if (this.onerror) {
						this.onerror(this, "No loop top marked to return to.");
						return -1;
					}
				}
			}
			if (this.onpostinst) {
				this.onpostinst(this);
			}

			this._lpc = this._pc;
			++this._pc;
			setTimeout(this._stepper, 0);
		}
		// done;
	},
	_find_jump_point: function(start) {
		var tpc = start;
		while (tpc < this._code.length) {
			if (this._code[tpc] === ']') {
				if (this._opens > 0) {
					this._opens--;
				}
				else {
					this._opens = 0;
					return tpc;
				}
			}
			else if (this._code[tpc] === '[') {
				this._opens++;
			}
			tpc++;
		}
		return -1;
	}
};