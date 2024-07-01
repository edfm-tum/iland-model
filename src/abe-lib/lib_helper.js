/** Helper functions
* ABE Library
*/

function initStandObj() {
    if (typeof stand.obj === 'object' && stand.obj !== null) return;

    stand.obj = {};
    stand.obj.act = {};

}

function mergeOptions(defaults, options) {
    const merged = {};
    for (const key in defaults) {
        merged[key] = options[key] !== undefined ? options[key] : defaults[key];
    }
    return merged;
}

function buildProgram(...concepts) {
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
