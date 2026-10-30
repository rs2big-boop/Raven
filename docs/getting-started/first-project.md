# Building Your First Project

Learn how to initialize, structure, and run a production-ready Raven project.

---

## 1. Project Initialization

Use `raven init` to scaffold a structured project:
```bash
raven init my_service
cd my_service
```

This creates:
```
my_service/
├── raven.json      # Project manifest & dependencies
└── src/
    └── main.rvn    # Application entry point
```

The generated `raven.json`:
```json
{
  "name": "my_service",
  "version": "1.0.0",
  "entry": "src/main.rvn",
  "dependencies": {}
}
```

---

## 2. Writing Concurrent Application Code

Open `src/main.rvn` and build a concurrent task runner using CSP channels and WaitGroups:

```raven
import "stdlib/sync";
import "stdlib/time";

fn worker(id, jobs, results, wg) {
    loop {
        let job = jobs.recv_timeout(100);
        if (job == nil) {
            break;
        }
        print("[Worker " + str(id) + "] Processing job #" + str(job));
        sleep(50);
        results.send("Result for job #" + str(job) + " processed by worker " + str(id));
    }
    wg.done();
}

fn main() {
    print("🚀 Initializing Raven Service...");
    
    let jobs = create_channel(10);
    let results = create_channel(10);
    let wg = create_wait_group();

    let num_workers = 3;
    let i = 0;
    while (i < num_workers) {
        let worker_id = i + 1;
        wg.add(1);
        go(fn() { worker(worker_id, jobs, results, wg); });
        i = i + 1;
    }

    // Dispatch 6 tasks
    let task_id = 1;
    while (task_id <= 6) {
        jobs.send(task_id);
        task_id = task_id + 1;
    }

    // Wait for all workers to finish
    wg.wait();
    print("✅ All jobs completed successfully!");
}

main();
```

---

## 3. Running & Benchmarking

Execute the project:
```bash
raven run src/main.rvn
```

Benchmark execution performance:
```bash
raven bench src/main.rvn
```

Generate automated API documentation:
```bash
raven doc src/main.rvn
```
