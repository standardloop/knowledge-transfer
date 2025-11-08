# Creating Kind Clusters with go-task (Taskfile)

## GitHub

-

## Prerequisites

- https://medium.com/@standardloop/using-taskfile-to-launch-docker-via-rancher-desktop-26cecd4007b6

## Current Taskfile

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
    silent: true
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

So running `task` should show the following:

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

## Kind

### Create a Directory to Store clusters

I'm going to config my Taskfile in a way to support deploying multiple clusters

Let's create a folder, I will name it `clusters` where I will store a config file for each cluster I want to deploy.

```sh
$ mkdir clusters
```

I will then create my first cluster config file:
```sh
$ touch clusters/slke-1.yaml
```
and place the following contents into the file:

```yaml
---
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
name: slke-1
nodes:
  - role: control-plane
    image: kindest/node:v1.34.0
  - role: worker
    image: kindest/node:v1.34.0
```

### Create a Task to deploy the clusters

Taskfile allows you to create variables for your tasks to reference.

I will make one variable to reference the folder name and then another for all the cluster files within the folder.

The `CLUSTERS` variable will be a [dynamic variable](https://taskfile.dev/docs/guide#dynamic-variables) where the value will be treated as a command and the output assigned.

I will use `find` command to essentially list all the files in the clusters directory.

I strongly recomned to use `find` instead of `ls`, which is a command you would normally use to list files in a directory.

```yaml
vars:
  CLUSTERS_FOLDER: clusters
  CLUSTERS:
    sh: find ./{{.CLUSTERS_FOLDER}} -maxdepth 1 -type f
```

`-maxdepth 1` means `find` will only check one directory deep.

`-type f` means `find` will only search for files, and not directories.
