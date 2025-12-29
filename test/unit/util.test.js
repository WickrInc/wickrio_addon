const { parsePositiveInt, parseTtl, parseBor } = require('../../src/util');

describe('parsePositiveInt', () => {
  describe('valid positive integers', () => {
    const validInputs = [
      { input: '0', expected: 0, description: '"0" as 0' },
      { input: '1', expected: 1, description: '"1" as 1' },
      { input: '100', expected: 100, description: '"100" as 100' },
      { input: '999', expected: 999, description: '"999" as 999' },
      { input: '1000000', expected: 1000000, description: 'large numbers correctly' }
    ];

    validInputs.forEach(({ input, expected, description }) => {
      it(`should parse ${description}`, () => {
        expect(parsePositiveInt(input)).toBe(expected);
      });
    });

    it('should parse numbers with leading zeros', () => {
      expect(parsePositiveInt('001')).toBe(1);
      expect(parsePositiveInt('0100')).toBe(100);
    });
  });

  describe('invalid inputs - negative numbers', () => {
    it('should throw error for "-1"', () => {
      expect(() => parsePositiveInt('-1')).toThrow('Value must be a positive number');
    });

    it('should throw error for "-100"', () => {
      expect(() => parsePositiveInt('-100')).toThrow('Value must be a positive number');
    });
  });

  describe('invalid inputs - non-numeric strings', () => {
    it('should throw error for "abc"', () => {
      expect(() => parsePositiveInt('abc')).toThrow('Value must be a positive number');
    });

    it('should parse "12abc" as 12 (parseInt stops at first non-digit)', () => {
      expect(parsePositiveInt('12abc')).toBe(12);
    });

    it('should throw error for "abc12"', () => {
      expect(() => parsePositiveInt('abc12')).toThrow('Value must be a positive number');
    });

    it('should parse "12 34" as 12 (parseInt stops at space)', () => {
      expect(parsePositiveInt('12 34')).toBe(12);
    });
  });

  describe('invalid inputs - decimals', () => {
    it('should parse "1.5" as 1 (parseInt behavior)', () => {
      expect(parsePositiveInt('1.5')).toBe(1);
    });

    it('should parse "99.99" as 99 (parseInt behavior)', () => {
      expect(parsePositiveInt('99.99')).toBe(99);
    });
  });

  describe('edge cases - empty and whitespace', () => {
    it('should return undefined for empty string', () => {
      expect(parsePositiveInt('')).toBeUndefined();
    });

    it('should throw error for whitespace only string', () => {
      expect(() => parsePositiveInt('   ')).toThrow('Value must be a positive number');
    });
  });

  describe('non-string inputs', () => {
    const nonStringInputs = [
      { value: null, description: 'null' },
      { value: undefined, description: 'undefined' },
      { value: 123, description: 'number' },
      { value: {}, description: 'object' },
      { value: [], description: 'array' },
      { value: true, description: 'boolean' }
    ];

    nonStringInputs.forEach(({ value, description }) => {
      it(`should return undefined for ${description}`, () => {
        expect(parsePositiveInt(value)).toBeUndefined();
      });
    });
  });
});

describe('parseTtl', () => {
  it('should return same result as parsePositiveInt for valid input', () => {
    expect(parseTtl('100')).toBe(100);
    expect(parseTtl('')).toBeUndefined();
  });

  it('should throw TTL-specific error message', () => {
    expect(() => parseTtl('-1')).toThrow('TTL must be a positive number');
    expect(() => parseTtl('abc')).toThrow('TTL must be a positive number');
  });
});

describe('parseBor', () => {
  it('should return same result as parsePositiveInt for valid input', () => {
    expect(parseBor('100')).toBe(100);
    expect(parseBor('')).toBeUndefined();
  });

  it('should throw BOR-specific error message', () => {
    expect(() => parseBor('-1')).toThrow('BOR must be a positive number');
    expect(() => parseBor('abc')).toThrow('BOR must be a positive number');
  });
});
