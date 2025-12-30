/**
 * parsePositiveInt parses a string and returns its value as an integer. If the parsed
 * string is not a valid positive integer, an error is thrown. If the provided value is
 * not a string, undefined is returned.
 *
 * @param {string} string
 * @returns int|undefined
 */
function parsePositiveInt(string) {
  if (typeof string === 'string' && string.length > 0) {
    const parsed = parseInt(string)

    if (isNaN(parsed) || parsed < 0) {
      throw 'Value must be a positive number'
    }

    return parsed
  }

  return undefined
}

/**
 * parseTtl wraps parsePositiveInt just to provide a more meaningful error message
 *
 * @param {string} string
 * @returns int|undefined
 */
function parseTtl(string) {
  try {
    return parsePositiveInt(string)
  } catch {
    throw 'TTL must be a positive number'
  }
}

/**
 * parseBor wraps parsePositiveInt just to provide a more meaningful error message
 *
 * @param {string} string
 * @returns int|undefined
 */
function parseBor(string) {
  try {
    return parsePositiveInt(string)
  } catch {
    throw 'BOR must be a positive number'
  }
}

module.exports = {
  parsePositiveInt,
  parseTtl,
  parseBor,
}
