# Deploying a Helm Chart with Taskfile

## GitHub

- https://github.com/standardloop/knowledge-transfer/tree/main/05-deploy-a-helm-chart-with-taskfile

## Prerequisites

- https://medium.com/@standardloop/using-taskfile-to-create-multiple-kind-clusters-0d9e4f4ad6d0

## Current Taskfile

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

## Deploy a Helm Chart, example: metrics-server

[Metrics Server](https://github.com/kubernetes-sigs/metrics-server) is a really common application you will deploy to a cluster. It allows you to use the HPA and VPA resources

First I will create a values file for the helm chart:

```sh
$ touch metrics-server.values.yaml
```

and I will place the following contents in it:
```yaml
---
args:
  - --cert-dir=/tmp
  - --kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname
  - --kubelet-use-node-status-port
  - --metric-resolution=15s
  - --kubelet-insecure-tls
```

Then let's add some variables to the Taskfile
```yaml
vars:
  METRICS_SERVER_NS: kube-system
  METRICS_SERVER_VERSION: 3.13.0
```
At the time of writing this article, `3.13.0` is the most recent chart, but always check the latest [here](https://artifacthub.io/packages/helm/metrics-server/metrics-server).

### Create a Task to deploy

```yaml
  metrics-server:deploy:
    silent: true
    deps:
      - cloud-provider-kind:run
    cmds:
      - |
        helm repo add metrics-server https://kubernetes-sigs.github.io/metrics-server/
        helm upgrade --install metrics-server metrics-server/metrics-server \
          -n {{.METRICS_SERVER_NS}} \
          --values metrics-server.values.yaml \
          --version {{.METRICS_SERVER_VERSION}} \
          --wait
```

And then running the task:

```sh
$ task metrics-server:deploy
task: Task "clusters:create" is up to date
task: Task "cloud-provider-kind:run" is up to date
"metrics-server" already exists with the same configuration, skipping
Release "metrics-server" does not exist. Installing it now.
NAME: metrics-server
LAST DEPLOYED: Sun Nov  9 16:13:35 2025
NAMESPACE: kube-system
STATUS: deployed
REVISION: 1
TEST SUITE: None
NOTES:
***********************************************************************
* Metrics Server                                                      *
***********************************************************************
  Chart version: 3.13.0
  App version:   0.8.0
  Image tag:     registry.k8s.io/metrics-server/metrics-server:v0.8.0
***********************************************************************
```

### Status

There are 2 ways to do this.
1) use helm template and then kubectl diff

```yaml
    status:
      - |
        helm template metrics-server metrics-server/metrics-server \
          -n {{.METRICS_SERVER_NS}} \
          --is-upgrade \
          --no-hooks \
          --values metrics-server.values.yaml \
          --version {{.METRICS_SERVER_VERSION}} | kubectl diff -f -
```

2) use the helm diff plugin

You can install with the following:
```sh
$ helm plugin install https://github.com/databus23/helm-diff
```

```yaml
    status:
      - |
        helm diff upgrade metrics-server metrics-server/metrics-server \
          -n {{.METRICS_SERVER_NS}} \
          --values metrics-server.values.yaml \
          --version {{.METRICS_SERVER_VERSION}}
```

Running the task again we see:

```sh
$ task metrics-server:deploy
task: Task "clusters:create" is up to date
task: Task "cloud-provider-kind:run" is up to date
task: Task "metrics-server:deploy" is up to date
```

### Clean task


```yaml
  metrics-server:clean:
    cmds:
      - helm uninstall metrics-server -n {{.METRICS_SERVER_NS}}
```

Really simple 😀

## The Full Taskfile

```yaml
---
version: '3'

vars:
  CLUSTERS_FOLDER: clusters
  CLUSTERS:
    sh: find ./{{.CLUSTERS_FOLDER}} -maxdepth 1 -type f
  METRICS_SERVER_NS: kube-system
  METRICS_SERVER_VERSION: 3.13.0

tasks:
  default:
    aliases: [all, make]
    deps:
      - metrics-server:deploy

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

  metrics-server:deploy:
    deps:
      - cloud-provider-kind:run
    cmds:
      - |
        helm repo add --force-update metrics-server https://kubernetes-sigs.github.io/metrics-server/
        helm upgrade --install metrics-server metrics-server/metrics-server \
          -n {{.METRICS_SERVER_NS}} \
          --values metrics-server.values.yaml \
          --version {{.METRICS_SERVER_VERSION}} \
          --wait
    status:
      - |
        helm template metrics-server metrics-server/metrics-server \
          -n {{.METRICS_SERVER_NS}} \
          --is-upgrade \
          --no-hooks \
          --values metrics-server.values.yaml \
          --version {{.METRICS_SERVER_VERSION}} | kubectl diff -f -
    # status:
    #   - |
    #     helm diff upgrade metrics-server metrics-server/metrics-server \
    #       -n {{.METRICS_SERVER_NS}} \
    #       --values metrics-server.values.yaml \
    #       --version {{.METRICS_SERVER_VERSION}}

  metrics-server:clean:
    cmds:
      - helm uninstall metrics-server -n {{.METRICS_SERVER_NS}}

  clean:
    aliases: [delete]
    ignore_error: true
    cmds:
      - task: metrics-server:clean
      - task: cloud-provider-kind:clean
      - task: clusters:clean
      - task: rancher:clean
```
