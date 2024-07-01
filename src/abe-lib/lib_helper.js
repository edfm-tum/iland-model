/** Helper functions
* ABE Library
*/

function initStandObj() {
    if (typeof stand.obj === 'object' && stand.obj !== null) return;

    stand.obj = {};
    stand.obj.act = {};

}

/* helper functions within the library */


lib.mergeOptions = function(defaults, options) {
    const merged = {};
    for (const key in defaults) {
        merged[key] = options[key] !== undefined ? options[key] : defaults[key];
    }
    return merged;
}

lib.buildProgram = function (...concepts) {
  const program = {};
  for (const conceptResult of concepts) {
    if (Array.isArray(conceptResult)) { // Multiple results
      conceptResult.forEach((activity, index) => {
          const key = `${conceptResult[0].type}${index + 1}`; // Or your naming logic
          program[key] = activity;
      });
    } else { // Single result
      program[conceptResult.type] = conceptResult;
    }
  }
  return program;
}

lib.loglevel = 0; // 0: none, 1: normal, 2: debug
lib.log = function(str) {
    if (lib.loglevel > 0)
        console.log(str);
}
lib.dbg = function(str) {
    if (lib.loglevel > 1)
        console.log(str);
}

lib.createSTP = function(stp_name, ...concepts) {
    const program = {};
    for (const conceptResult of concepts) {
      if (Array.isArray(conceptResult)) { // Multiple results
        conceptResult.forEach((activity, index) => {
            const key = `${conceptResult[0].type}${index + 1}`; // Or your naming logic
            program[key] = activity;
        });
      } else { // Single result
        program[conceptResult.type] = conceptResult;
      }
    }

    try {
       const test = fmengine.stpByName(stp_name);
       // the STP already exists, so update the program
       fmengine.updateManagement(program, stp_name);
    } catch (error) {
        // the STP does not yet exist, add to fmengine
        fmengine.addManagement(program, stp_name);
    }
}
