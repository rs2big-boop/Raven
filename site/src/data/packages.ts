export interface RavenPackage {
  id: string;
  name: string;
  version: string;
  description: string;
  author: string;
  repository: string;
  category: 'web' | 'concurrency' | 'database' | 'security' | 'cli' | 'data' | 'tools';
  stars: number;
  downloads: number;
  tags: string[];
  installCommand: string;
  featured?: boolean;
}

export const COMMUNITY_PACKAGES: RavenPackage[] = [
  {
    id: 'raven-router',
    name: 'raven-router',
    version: '1.2.0',
    description: 'High-throughput HTTP trie router with regex path params, CORS middleware, and request pipeline.',
    author: 'raven-core',
    repository: 'github.com/raven-ecosystem/raven-router',
    category: 'web',
    stars: 342,
    downloads: 14200,
    tags: ['web', 'routing', 'http', 'middleware'],
    installCommand: 'raven pkg add raven-router',
    featured: true
  },
  {
    id: 'raven-jwt',
    name: 'raven-jwt',
    version: '0.9.4',
    description: 'HMAC-SHA256 and RSA JSON Web Token encoder, decoder, and claims verification with clock skew tolerance.',
    author: 'security-guild',
    repository: 'github.com/raven-ecosystem/raven-jwt',
    category: 'security',
    stars: 215,
    downloads: 9800,
    tags: ['jwt', 'security', 'crypto', 'tokens'],
    installCommand: 'raven pkg add raven-jwt',
    featured: true
  },
  {
    id: 'raven-pg',
    name: 'raven-pg',
    version: '1.0.1',
    description: 'Pure Raven wire protocol client for PostgreSQL with connection pooling, prepared statements, and SSL.',
    author: 'db-team',
    repository: 'github.com/raven-ecosystem/raven-pg',
    category: 'database',
    stars: 198,
    downloads: 8400,
    tags: ['postgres', 'sql', 'database', 'pooling'],
    installCommand: 'raven pkg add raven-pg',
    featured: true
  },
  {
    id: 'raven-redis',
    name: 'raven-redis',
    version: '0.8.2',
    description: 'RESP3 compliant in-memory cache client supporting Pub/Sub, pipelining, and automatic reconnect.',
    author: 'caching-labs',
    repository: 'github.com/raven-ecosystem/raven-redis',
    category: 'database',
    stars: 184,
    downloads: 7650,
    tags: ['redis', 'cache', 'key-value', 'resp3'],
    installCommand: 'raven pkg add raven-redis'
  },
  {
    id: 'raven-worker-pool',
    name: 'raven-worker-pool',
    version: '1.0.0',
    description: 'Scalable CSP thread-pool with dynamic worker sizing, backpressure, task cancellation, and latency metrics.',
    author: 'concurrency-devs',
    repository: 'github.com/raven-ecosystem/raven-worker-pool',
    category: 'concurrency',
    stars: 256,
    downloads: 8900,
    tags: ['concurrency', 'threads', 'csp', 'pool'],
    installCommand: 'raven pkg add raven-worker-pool',
    featured: true
  },
  {
    id: 'raven-cli-ui',
    name: 'raven-cli-ui',
    version: '0.5.0',
    description: 'Terminal UI toolkit: animated spinners, progress bars, interactive multi-select menus, and table formatters.',
    author: 'cli-wizards',
    repository: 'github.com/raven-ecosystem/raven-cli-ui',
    category: 'cli',
    stars: 289,
    downloads: 11200,
    tags: ['cli', 'terminal', 'ui', 'colors', 'prompts'],
    installCommand: 'raven pkg add raven-cli-ui',
    featured: true
  },
  {
    id: 'raven-validator',
    name: 'raven-validator',
    version: '1.1.0',
    description: 'Declarative schema validator for incoming JSON payloads, query strings, and environment configs.',
    author: 'ran-sh',
    repository: 'github.com/raven-ecosystem/raven-validator',
    category: 'data',
    stars: 142,
    downloads: 6500,
    tags: ['validation', 'schema', 'types', 'forms'],
    installCommand: 'raven pkg add raven-validator'
  },
  {
    id: 'raven-yaml',
    name: 'raven-yaml',
    version: '0.4.1',
    description: 'Full YAML 1.2 parser and serializer with support for multi-document streams and anchors.',
    author: 'infra-tools',
    repository: 'github.com/raven-ecosystem/raven-yaml',
    category: 'data',
    stars: 118,
    downloads: 5100,
    tags: ['yaml', 'parser', 'config'],
    installCommand: 'raven pkg add raven-yaml'
  },
  {
    id: 'raven-test-runner',
    name: 'raven-test-runner',
    version: '1.0.2',
    description: 'Advanced testing harness with parallel suite execution, snapshot assertions, and JUnit XML reports.',
    author: 'qa-guild',
    repository: 'github.com/raven-ecosystem/raven-test-runner',
    category: 'tools',
    stars: 167,
    downloads: 7200,
    tags: ['testing', 'bdd', 'assertions', 'ci'],
    installCommand: 'raven pkg add raven-test-runner'
  }
];
