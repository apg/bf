Function.prototype.bind = function(scope) {
  var _function = this;

  return function() {
    return _function.apply(scope, arguments);
  }
};

BrainFuckStatus = {
  ERROR: -1,
  STOPPED: 0,
  RUNNING: 1,
  COMPLETE: 2
};

BrainFuck = function(inp, out, heapsize) {
  this._machinestatus = BrainFuckStatus.STOPPED; // status
  this._pc = 0;
  this._lpc = 0;
  this._heap = [];
  this._heapsize = heapsize;
  this._dp = 0;
  this._opens = 0;
  this._loopstack = [];
  this.getinput = inp;
  this.output = out;
  this._initHeap();
  this._breakpoints = {};
  this._stepper = this.step.bind(this);
  this._execer = this.exec.bind(this);
};

BrainFuck.prototype = {
  reset: function() {
    this._machinestatus = BrainFuckStatus.STOPPED;
    this._lpc = 0;
    this._pc = 0;
    this._heap = [];
    this._dp = 0;
    this._opens = 0;
    this._loopstack = [];
    this._initHeap();
    if (this.onreset) {
      this.onreset(this);
    }
  },
  _initHeap: function() {
    for (var i = 0; i < this._heapsize; i++) {
      this._heap[i] = 0;
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
  lastDP: function() {
    return this._ldp;
  },
  lastAtDP: function() {
    return this._heap[this._ldp];
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
  status: function() {
    return this._machinestatus;
  },
  toggleBreakPoint: function(offset) {
    if (this._breakpoints[offset]) {
      delete this._breakpoints[offset];
    }
    else {
      this._breakpoints[offset] = 1;
    }
  },
  load: function(code) {
    this.reset();
    this._code = code;
    if (this.onpreexec) {
      this.onpreexec(this);
    }
    this._machinestatus = BrainFuckStatus.RUNNING;
  },
  exec: function() {
    console.log('exec called');
    if (this._breakpoints[this._pc]) {
      if (this.onbreakpoint) {
        this.onbreakpoint(this);
      }
    }
    else {
      this.step();
      if (this._machinestatus == BrainFuckStatus.RUNNING) {
        setTimeout(this._execer, 0);
      }
    }
    return this._machinestatus;
  },
  step: function() {
    if (this._pc < this._code.length) {
      if (this.onpreinst) {
	 this.onpreinst(this);
      }

      this._ldp = this._dp;

      var inst = this._code[this._pc];
      if (inst === '>') {
	 var ndp = this._dp + 1;
	 if (ndp < this._heapsize) {
	   this._dp = ndp;
	 }
	 else {
          this._machinestatus = BrainFuckStatus.ERROR;
	   if (this.onerror) {
	     this.onerror(this, "Heap out of bounds.");
	   }
	   return BrainFuckStatus.ERROR;
	 }
      }
      else if (inst === '<') {
	 var ndp = this._dp - 1;
	 if (ndp >= 0) {
	   this._dp = ndp;
	 }
	 else {
          this._machinestatus = BrainFuckStatus.ERROR;
	   if (this.onerror) {
	     this.onerror(this, "Heap out of bounds.");
	   }
	   return BrainFuckStatus.ERROR;
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
	   var newpc = this._find_jump_point(this._pc);
	   if (newpc < 0) {
            this._machinestatus = BrainFuckStatus.ERROR;
	     if (this.onerror) {
		this.onerror(this, "No end loop instruction found after loop open.");
	     }
	     return BrainFuckStatus.ERROR;
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
            this._machinestatus = BrainFuckStatus.RUNNING;
	     if (this.onpostinst) {
		this.onpostinst(this);
	     }
	     return BrainFuckStatus.RUNNING;
           //continue; // skip the increment
	   }
	 }
	 else {
          this._machinestatus = BrainFuckStatus.ERROR;
	   if (this.onerror) {
	     this.onerror(this, "No loop top marked to return to.");
	   }
	   return BrainFuckStatus.ERROR;
	 }
      }

      this._lpc = this._pc;
      ++this._pc;

      this._machinestatus = BrainFuckStatus.RUNNING;
      if (this.onpostinst) {
	 this.onpostinst(this);
      }
      return BrainFuckStatus.RUNNING;
    }
    this._machinestatus = BrainFuckStatus.COMPLETE;
    return BrainFuckStatus.COMPLETE;
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
