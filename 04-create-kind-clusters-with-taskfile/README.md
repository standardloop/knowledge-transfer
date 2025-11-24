# Creating Kind Clusters with go-task (Taskfile)

## GitHub

- https://github.com/standardloop/knowledge-transfer/tree/main/04-create-kind-clusters-with-taskfile

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
      - rancher:run

  rancher:run:
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

  rancher:clean:
    cmds:
      - rdctl shutdown

  clean:
    aliases: [delete]
    ignore_error: true
    cmds:
      - task: rancher:clean
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

#### Dynamic Variable for Finding Cluster Config File(s)

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

We can create a `test` task to just check if the variable is being set properly:

```yaml
  test:
    silent: true
    cmds:
      - |
        {{range .CLUSTERS | splitArgs}}
          echo "{{.}}"
        {{end}}
```

So running `task test`, we should see the following:

```sh
$ task test
./clusters/slke-1.yaml
```

I'm using templates for the loop, you can read more about it in the docs [here](https://taskfile.dev/docs/reference/templating#templating-reference).

#### Task to deploy the Clusters

Before creating the cluster, I will get the name of the cluster from the config file using [yq](https://github.com/mikefarah/yq).

I will then use `kubectl cluster-info` to see if a cluster was already created with the same name.

If it's not created then I will use `kind create cluster` to create the cluster with the config file path we got from or dynamic `CLUSTERS` variable.

```yaml
  clusters:create:
    run: once
    silent: true
    deps:
      - rancher:run
    cmds:
      - |
        {{range .CLUSTERS | splitArgs}}
          cluster_name=$(yq '.name' {{.}})
          if ! kubectl cluster-info --context kind-${cluster_name} > /dev/null 2>&1
          then
            kind create cluster --config={{.}}
          else
            echo "$cluster_name already created"
          fi
        {{end}}
```

Running the task we can see the following:

```sh
$ task clusters:create
Creating cluster "slke-1" ...
 ✓ Ensuring node image (kindest/node:v1.34.0) 🖼
 ✓ Preparing nodes 📦 📦
 ✓ Writing configuration 📜
 ✓ Starting control-plane 🕹️
 ✓ Installing CNI 🔌
 ✓ Installing StorageClass 💾
 ✓ Joining worker nodes 🚜
Set kubectl context to "kind-slke-1"
You can now use your cluster with:

kubectl cluster-info --context kind-slke-1

Have a question, bug, or feature request? Let us know! https://kind.sigs.k8s.io/#community 🙂
```

If we add another file to our clusters folder, example `slke-2.yaml` with the following contents:
```yaml
---
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
name: slke-2
nodes:
  - role: control-plane
    image: kindest/node:v1.34.0
  - role: worker
    image: kindest/node:v1.34.0
```

We can run the task again to see the following:

```sh
$ task clusters:create
slke-1 already created
Creating cluster "slke-2" ...
 ✓ Ensuring node image (kindest/node:v1.34.0) 🖼
 ✓ Preparing nodes 📦 📦
 ✓ Writing configuration 📜
 ✓ Starting control-plane 🕹️
 ✓ Installing CNI 🔌
 ✓ Installing StorageClass 💾
 ✓ Joining worker nodes 🚜
Set kubectl context to "kind-slke-2"
You can now use your cluster with:

kubectl cluster-info --context kind-slke-2

Have a nice day! 👋
```

Note: if you try to make too many clusters, you may get a cgroup error

---

Now let's add a status:

```yaml
  clusters:create:
    run: once
    deps:
      - rancher:run
    cmds:
      - |
        {{range .CLUSTERS | splitArgs}}
          cluster_name=$(yq '.name' {{.}})
          if ! kubectl cluster-info --context kind-${cluster_name} > /dev/null 2>&1
          then
            kind create cluster --config={{.}}
          else
            echo "$cluster_name already created"
          fi
        {{end}}
    status:
      - |
        {{range .CLUSTERS | splitArgs}}
          cluster_name=$(yq '.name' {{.}})
          kubectl cluster-info --context kind-${cluster_name}
        {{end}}
```

The status will check all the clusters and ensure the cluster is reachable with `kubectl`.

Running the task again we see the following:

```
$ task clusters:create
task: Task "clusters:create" is up to date
```

---

Finally let's add a clean task

```yaml
  clusters:clean:
    silent: true
    cmds:
      - kind delete clusters --all
```

This one is really simple because `kind` has a flag to delete all clusters.

There is no need to loop through all our config for this one.

Running it we see:
```sh
$ task clusters:clean
Deleted nodes: ["slke-1-worker" "slke-1-control-plane"]
Deleted nodes: ["slke-2-control-plane" "slke-2-worker"]
Deleted clusters: ["slke-1" "slke-2"]
```

If we didn't have any clusters and we ran it we will see this:
```sh
$ task clusters:clean
Deleted clusters: []
```
So like I stated before there is no concern to loop through our config.


### Create a Task to run `cloud-provider-kind`

`cloud-provider-kind` allows you to use a Kubernetes Service type LoadBalancer.

```yaml
  cloud-provider-kind:run:
    run: once
    deps:
      - clusters:create
    interactive: true
    cmds:
      - sudo -v
      - bash -c 'nohup sudo cloud-provider-kind >/dev/null 2>&1 &'
```

since this command requires `sudo` access, I will first use `sudo -v` to all the user to pre-authorize sudo commands.

I will then use `nohup` in combination  with `&` so the program is spawned in the background.

If you do not do this, to task is considered the parent of the program and when the task finishes it will remove it's children.

If you ever need to undo `sudo -v` you can use `sudo -k`

Running the task we will see:

```sh
$ task cloud-provider-kind:run
task: Task "clusters:create" is up to date
task: [cloud-provider-kind:run] sudo -v
Password:
task: [cloud-provider-kind:run] bash -c 'nohup sudo cloud-provider-kind >/dev/null 2>&1 &'
```

You can then use `ps -A` to see the process running or use `pgrep`

Speaking of `pgrep` let's use it as our status checker:

```yaml
    status:
      - pgrep sudo cloud-provider-kind
```

So running the task again we will see:
```sh
$ task cloud-provider-kind:run
task: Task "clusters:create" is up to date
task: Task "cloud-provider-kind:run" is up to date
```

---

For the clean task

```yaml
  cloud-provider-kind:clean:
    silent: true
    interactive: true
    cmds:
      - |
        if pgrep sudo cloud-provider-kind > /dev/null 2>&1
        then
          sudo -v
          sudo pkill cloud-provider-kind
        else
          echo "cloud-provider-kind is not running"
        fi
```

Let's check if it is running, if it is then we will remove it.

---

## The Whole Taskfile

```yaml
---
version: '3'

vars:
  CLUSTERS_FOLDER: clusters
  CLUSTERS:
    sh: find ./{{.CLUSTERS_FOLDER}} -maxdepth 1 -type f

tasks:
  default:
    aliases: [all, make]
    deps:
      - cloud-provider-kind:run

  test:
    silent: true
    cmds:
      - |
        {{range .CLUSTERS | splitArgs}}
          echo "{{.}}"
        {{end}}

  rancher:run:
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

  rancher:clean:
    prompt: Do you want to update quit Rancher Desktop?
    cmds:
      - rdctl shutdown

  clusters:create:
    run: once
    deps:
      - rancher:run
    cmds:
      - |
        {{range .CLUSTERS | splitArgs}}
          cluster_name=$(yq '.name' {{.}})
          if ! kubectl cluster-info --context kind-${cluster_name} > /dev/null 2>&1
          then
            kind create cluster --config={{.}}
          else
            echo "$cluster_name already created"
          fi
        {{end}}
    status:
      - |
        {{range .CLUSTERS | splitArgs}}
          cluster_name=$(yq '.name' {{.}})
          kubectl cluster-info --context kind-${cluster_name}
        {{end}}

  clusters:clean:
    silent: true
    cmds:
      - kind delete clusters --all

  cloud-provider-kind:run:
    run: once
    deps:
      - clusters:create
    interactive: true
    cmds:
      - sudo -v
      - bash -c 'nohup sudo cloud-provider-kind >/dev/null 2>&1 &'
    status:
      - pgrep sudo cloud-provider-kind

  cloud-provider-kind:clean:
    silent: true
    interactive: true
    cmds:
      - |
        if pgrep sudo cloud-provider-kind > /dev/null 2>&1
        then
          sudo -v
          sudo pkill cloud-provider-kind
        else
          echo "cloud-provider-kind is not running"
        fi

  cloud-provider-kind:restart:
    interactive: true
    cmds:
      - task: clean-cloud-provider-kind
      - task: run-cloud-provider-kind

  clean:
    aliases: [delete]
    ignore_error: true
    cmds:
      - task: cloud-provider-kind:clean
      - task: clusters:clean
      - task: rancher:clean
```
