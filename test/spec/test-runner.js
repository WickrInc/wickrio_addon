const yaml = require('js-yaml');
const fs = require('fs');

/**
 * Spec Test Runner
 * Parses YAML test definitions and generates dynamic Jest test cases
 */
class SpecTestRunner {
  constructor(wickrIOAddonClass) {
    this.WickrIOAddon = wickrIOAddonClass;
    // This is the instance of the wickrIOAddonClass we set up mocks on later
    this.wickrIOAddon = null;
    this.mockQueueCommands = null;
  }

  /**
   * Load and parse YAML test definitions
   *
   * TODO: Support loading multiple YAML files for test definitions
   */
  loadTestDefinitions(yamlFilePath) {
    const yamlContent = fs.readFileSync(yamlFilePath, 'utf8');
    const parsed = yaml.load(yamlContent);
    
    // Convert string "undefined" to actual undefined values
    return this.convertUndefinedValues(parsed);
  }

  /**
   * Recursively convert string "undefined" to actual undefined values
   */
  convertUndefinedValues(obj) {
    if (obj === null || typeof obj !== 'object') {
      return obj === 'undefined' ? undefined : obj;
    }
    
    if (Array.isArray(obj)) {
      return obj.map(item => this.convertUndefinedValues(item));
    }
    
    const result = {};
    for (const [key, value] of Object.entries(obj)) {
      result[key] = this.convertUndefinedValues(value);
    }
    return result;
  }

  /**
   * Setup mock environment for each test
   */
  setupMocks() {
    this.wickrIOAddon = new this.WickrIOAddon(false); // debug off
    this.mockQueueCommands = {
      sendMessage: jest.fn().mockResolvedValue({ success: true, result: null })
    };

    // Initialize client with test username to set clientName
    this.wickrIOAddon.clientInit('test-bot', null, null, null, null);

    // Override queueCommands AFTER clientInit (which creates its own queueCommands)
    this.wickrIOAddon.queueCommands = this.mockQueueCommands;
  }

  /**
   * Generate Jest test cases from YAML definitions
   */
  generateTests(testDefinitions) {
    const tests = testDefinitions.tests;
    const self = this;

    Object.keys(tests).forEach(methodName => {
      describe(`${methodName} - Spec Tests`, () => {
        beforeEach(() => {
          self.setupMocks();
        });

        tests[methodName].forEach((testCase, index) => {
          const testName = testCase.case || `Test case ${index + 1}`;
          
          it(testName, async () => {
            await self.runTestCase(methodName, testCase);
          });
        });
      });
    });
  }

  /**
   * Execute a single test case
   */
  async runTestCase(methodName, testCase) {
    const method = this.wickrIOAddon[methodName];
    if (!method) {
      throw new Error(`Method ${methodName} not found on WickrIOAddon`);
    }

    // Handle methods that don't call sendMessage (like cmdGetClientInfo)
    if (testCase.expectResult !== undefined) {
      const result = await method.apply(this.wickrIOAddon, testCase.args);
      expect(result).toEqual(testCase.expectResult);
      return;
    }

    // Set up mock response for methods that call sendMessage
    const mockResponse = testCase.mockResponse || { success: true, result: null };
    this.mockQueueCommands.sendMessage.mockResolvedValue(mockResponse);

    if (testCase.throws) {
      await expect(
        method.apply(this.wickrIOAddon, testCase.args)
      ).rejects.toMatch(testCase.throws);
      return;
    }

    // Execute the method
    const result = await method.apply(this.wickrIOAddon, testCase.args);

    // Verify method was called
    expect(this.mockQueueCommands.sendMessage).toHaveBeenCalledTimes(1);

    // Get the sent command and parse it
    const sentCommand = this.mockQueueCommands.sendMessage.mock.calls[0][0];
    const commandObj = JSON.parse(sentCommand);

    if (testCase.expect) {
      if (testCase.exactMatch) {
        expect(commandObj).toEqual(testCase.expect);
      } else {
        // Partial match - only check specified fields
        expect(commandObj).toMatchObject(testCase.expect);
      }
    }

    // Verify fields that should NOT be present
    if (testCase.expectNotPresent) {
      testCase.expectNotPresent.forEach(field => {
        expect(commandObj).not.toHaveProperty(field);
      });
    }
  }

  /**
   * Create Jest test suite from YAML file
   */
  createTestSuite(yamlFilePath) {
    const testDefinitions = this.loadTestDefinitions(yamlFilePath);
    this.generateTests(testDefinitions);
  }
}

module.exports = SpecTestRunner;
