# Using Taskfile (go-task) with Rancher

## GitHub

- https://github.com/standardloop/knowledge-transfer/tree/main/03-running-rancher-with-go-task

## Install and Setup

### Rancher

Rancher Desktop is an app that provides container management and Kubernetes on the desktop. It is available for Mac (both on Intel and Apple Silicon), Windows, and Linux.


For this article, I will be using [Rancher Desktop](https://github.com/rancher-sandbox/rancher-desktop/).

You can install Rancher Desktop with `brew` like this:

```sh
$ brew install --cask rancher
```

For other installation methods you can read the docs [here](https://docs.rancherdesktop.io/getting-started/installation/).

### go-task (Taskfile)

[go-task](https://github.com/go-task/task) is a task runner / build tool that aims to be simpler and easier to use than, for example, GNU Make.

`task` uses `yaml` syntax.


You can install task with `brew` like this:

```sh
$ brew install go-task
$ which task
/opt/homebrew/bin/task
```

For more install options you can view the docs [here](https://taskfile.dev/docs/installation).

## Getting started

To generate a quick boilerplate taskfile you can run the following:

```sh
$ task --init
Taskfile created: Taskfile.yml
```

You can then run the taskfile like this:

```sh
$ task
Hello, World!
```

### Create a task to launch Rancher

```yaml
---
version: '3'

tasks:
  default:
    deps:
      - run-rancher

  run-rancher:
    run: once
    cmds:
      - rdctl start
```

This task can be run with:

```sh
$ task run-rancher
```

The problem is though, `rdctl start` will launch the app, but we do not know if the docker engine has started enough to use it

/* add photo here */

So after starting rancher let's add a loop to wait until docker is ready.

```yaml
---
version: '3'

tasks:
  default:
    deps:
      - run-rancher

  run-rancher:
    run: once
    cmds:
      - rdctl start
      - |
        until docker info > /dev/null 2>&1
        do
          echo "waiting for docker to come up...."
          sleep 5
        done
        echo "docker is ready!"
```

We can use `until` here to check `docker info` *until* it is up and ready.

Running the task we will see something like this:
```sh
$ task
INFO[0000] About to launch /usr/bin/open -a /Applications/Rancher Desktop.app ...
waiting for docker to come up....
waiting for docker to come up....
waiting for docker to come up....
waiting for docker to come up....
waiting for docker to come up....
docker is ready!
```

task has a [status feature](https://taskfile.dev/docs/guide#using-programmatic-checks-to-indicate-a-task-is-up-to-date) where it won't re-run a task if a condition is true.

For us, we don't want to re-launch rancher if we know docker is running and the rancher cli is able to view it's own settings. Really only the former matters, but I will add both.

The taskfile should now look like this:

```yaml
---
version: '3'

tasks:
  default:
    deps:
      - run-rancher

  run-rancher:
    run: once
    cmds:
      - rdctl start
      - |
        until docker info > /dev/null 2>&1
        do
          echo "waiting for docker to come up...."
          sleep 5
        done
        echo "docker is ready!"
    status:
      - rdctl list-settings > /dev/null 2>&1
      - docker info > /dev/null 2>&1
```

Now when we one `task` again we will see the following:

```sh
$ task
task: Task "run-rancher" is up to date
```

### Create a task to stop Rancher

In addition to `rdctl` having a start command, it also as a stop command which is `rdctl shutdown`.

Let's add a clean task using this:

```yaml
  clean-rancher:
    cmds:
      - rdctl shutdown
```

Running `task clean-rancher` should show the following:
```sh
$ task clean-rancher
task: [clean-rancher] rdctl shutdown
Shutting down.
```

Similar to the default task I made, I like to make a clean task:

```yaml
  clean:
    aliases: [delete]
    ignore_error: true
    cmds:
      - task: clean-rancher
```

Now you can run `task clean` or `task delete`

## The full Taskfile


```yaml
---
version: '3'

tasks:
  default:
    aliases: [all, make]
    deps:
      - run-rancher

  run-rancher:
    run: once
    cmds:
      - rdctl start
      - |
        until docker info > /dev/null 2>&1
        do
          echo "waiting for docker to come up...."
          sleep 5
        done
        echo "docker is ready!"
    status:
      - rdctl list-settings > /dev/null 2>&1
      - docker info > /dev/null 2>&1

  clean-rancher:
    cmds:
      - rdctl shutdown

  clean:
    aliases: [delete]
    ignore_error: true
    cmds:
      - task: clean-rancher
```
