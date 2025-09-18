const { WickrIOAddon } = require('../../src/wickrio_addon');
const SpecTestRunner = require('./test-runner');
const path = require('path');

jest.mock('../../src/zmq_commands');
jest.mock('../../src/sqs_commands');

const testRunner = new SpecTestRunner(WickrIOAddon);
const yamlPath = path.join(__dirname, 'test-definitions.yaml');
testRunner.createTestSuite(yamlPath);
