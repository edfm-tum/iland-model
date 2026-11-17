// Lightweight ES6 test harness for iLand QJSEngine API tests.
// Load at top of any test script with:
//   Globals.include('../../js/harness.js');
// Works identically in headless ilandc (run_tests.py) and interactively in the iLand GUI.

var Test = (function() {
  let _suiteName = 'js_suite';
  const _tests = [];

  function formatVal(v) {
    if (v === null) return 'null';
    if (v === undefined) return 'undefined';
    if (typeof v === 'string') return JSON.stringify(v);
    if (typeof v === 'object') {
      try { return JSON.stringify(v); } catch (e) { return String(v); }
    }
    return String(v);
  }

  function fail(msg) {
    throw new Error(msg || 'Assertion failed');
  }

  const assert = {
    ok: function(cond, msg) {
      if (!cond) {
        fail(msg || ('Expected truthy value, got ' + formatVal(cond)));
      }
    },

    eq: function(actual, expected, msg) {
      if (!Object.is(actual, expected)) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected ' + formatVal(expected) + ', got ' + formatVal(actual));
      }
    },

    neq: function(actual, expected, msg) {
      if (Object.is(actual, expected)) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected value != ' + formatVal(expected));
      }
    },

    approx: function(actual, expected, tol, msg) {
      if (typeof tol === 'string') {
        msg = tol;
        tol = 1e-6;
      }
      if (tol === undefined || tol === null) {
        tol = 1e-6;
      }
      if (typeof actual !== 'number' || !Number.isFinite(actual)) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected finite number near ' + expected + ', got ' + formatVal(actual));
      }
      if (Math.abs(actual - expected) > tol) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected ' + expected + ' +/- ' + tol + ', got ' + actual +
             ' (diff=' + Math.abs(actual - expected) + ')');
      }
    },

    gt: function(a, b, msg) {
      if (!(a > b)) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected ' + formatVal(a) + ' > ' + formatVal(b));
      }
    },

    gte: function(a, b, msg) {
      if (!(a >= b)) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected ' + formatVal(a) + ' >= ' + formatVal(b));
      }
    },

    lt: function(a, b, msg) {
      if (!(a < b)) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected ' + formatVal(a) + ' < ' + formatVal(b));
      }
    },

    lte: function(a, b, msg) {
      if (!(a <= b)) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected ' + formatVal(a) + ' <= ' + formatVal(b));
      }
    },

    contains: function(haystack, needle, msg) {
      let found = false;
      if (typeof haystack === 'string') {
        found = haystack.indexOf(needle) !== -1;
      } else if (Array.isArray(haystack)) {
        found = haystack.indexOf(needle) !== -1;
      } else if (haystack && typeof haystack.length === 'number') {
        for (let i = 0; i < haystack.length; ++i) {
          if (haystack[i] === needle) {
            found = true;
            break;
          }
        }
      }
      if (!found) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected ' + formatVal(haystack) + ' to contain ' + formatVal(needle));
      }
    },

    throws: function(fn, expectedSubstr, msg) {
      let caught = null;
      try {
        fn();
      } catch (e) {
        caught = e;
      }
      if (caught === null) {
        const prefix = msg ? (msg + ': ') : '';
        fail(prefix + 'expected function to throw, but it succeeded');
      }
      if (expectedSubstr) {
        const errStr = String(caught);
        if (errStr.indexOf(expectedSubstr) === -1) {
          const prefix = msg ? (msg + ': ') : '';
          fail(prefix + 'expected error matching ' + formatVal(expectedSubstr) +
               ', got ' + formatVal(errStr));
        }
      }
    }
  };

  function suite(name) {
    _suiteName = String(name);
  }

  function add(name, fn) {
    _tests.push({ name: String(name), fn: fn, only: false, skip: false });
  }

  function only(name, fn) {
    _tests.push({ name: String(name), fn: fn, only: true, skip: false });
  }

  function skip(name, fn) {
    _tests.push({ name: String(name), fn: fn, only: false, skip: true });
  }

  function formatError(err) {
    const base = (err && err.message) ? err.message : String(err);
    if (err && err.stack) {
      const firstLine = String(err.stack).split('\n')[0];
      if (firstLine && base.indexOf(firstLine) === -1) {
        return base + ' [' + firstLine + ']';
      }
    }
    return base;
  }

  function run() {
    const hasOnly = _tests.some(function(t) { return t.only; });
    const results = [];
    let passed = 0;
    let failed = 0;
    let skipped = 0;

    Globals.print('=== JS Suite: ' + _suiteName + ' ===');

    for (let i = 0; i < _tests.length; ++i) {
      const t = _tests[i];
      if (t.skip || (hasOnly && !t.only)) {
        skipped++;
        results.push({ name: t.name, status: 'skip', duration_ms: 0, error: '' });
        Globals.print('  [SKIP] ' + t.name);
        continue;
      }

      const t0 = Date.now();
      try {
        t.fn(assert);
        const dt = Date.now() - t0;
        passed++;
        results.push({ name: t.name, status: 'pass', duration_ms: dt, error: '' });
        Globals.print('  [PASS] ' + t.name + ' (' + dt + ' ms)');
      } catch (e) {
        const dt = Date.now() - t0;
        const msg = formatError(e);
        failed++;
        results.push({ name: t.name, status: 'fail', duration_ms: dt, error: msg });
        Globals.print('  [FAIL] ' + t.name + ': ' + msg);
      }
    }

    const total = passed + failed;
    const summary = {
      suite: _suiteName,
      total: total,
      passed: passed,
      failed: failed,
      skipped: skipped,
      tests: results
    };

    try {
      const outPath = Globals.defaultDirectory('temp') + 'js_test_results.json';
      Globals.saveTextFile(outPath, JSON.stringify(summary, null, 2));
    } catch (e) {
      Globals.print('Warning: could not write js_test_results.json: ' + e);
    }

    Globals.print('Summary (' + _suiteName + '): ' + passed + '/' + total + ' passed' +
                  (skipped > 0 ? (', ' + skipped + ' skipped') : '') + '.');

    if (failed > 0 || total === 0) {
      throw new Error('JS test suite "' + _suiteName + '" failed (' + failed + ' failure(s), ' + total + ' executed).');
    }
    return summary;
  }

  return {
    suite: suite,
    add: add,
    only: only,
    skip: skip,
    run: run
  };
})();
