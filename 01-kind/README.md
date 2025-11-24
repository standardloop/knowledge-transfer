# Introduction to Local Kubernetes Using Kind

## GitHub Repo

- https://github.com/standardloop/knowledge-transfer/tree/main/01-kind


## Background

*kind is a tool for running local Kubernetes clusters using Docker container “nodes”.
kind was primarily designed for testing Kubernetes itself, but may be used for local development or CI.*

Kind is part of the Kubernetes Special Interest Groups (SIGS)

Special Interest Groups in Kubernetes provide places for the community to focus development and discussion on a particular part of the project.

You can find the Kind source code here:
- https://github.com/kubernetes-sigs/kind

and the documentation here:
- https://kind.sigs.k8s.io/

## Prerequisites

### General Knowledge
- `docker`
- `brew` (or your preferred package manager)
- YAML
- `helm`
- `kubectl`

### Install and Setup

#### Docker
Have `docker` installed and have a docker engine running.

For this article, I will be using [Rancher Desktop](https://github.com/rancher-sandbox/rancher-desktop/).

You can install Rancher Desktop with `brew` like this:

```sh
$ brew install --cask rancher
```

You can check your `docker` version with the following command:
```sh
$ docker --version
Docker version 28.1.1-rd, build 4d7f01e
```

Make sure your `docker` engine is running.
You can check if it is with the following command:
```sh
$ docker info > /dev/null 2>&1
$ echo $?
0  # if it isn't running you should see a 1
```

#### `kind`

I recommend installing using a package manager such as `brew` for macOS:

```sh
$ brew install kind
$ which kind
/opt/homebrew/bin/kind
$ kind --version
kind version 0.30.0
```

## Creating a cluster with a yaml config file

- https://kind.sigs.k8s.io/docs/user/configuration/

### Background

I will naming my cluster `slke-1` (standardloop kubernetes engine 1).

You can create clusters without config files, but I always recommend getting in the habit of using them for repeatable behavior.

---

There are two kinds of node types, `control-plane` and `worker`.

A single `control-plane` node is always required.

If you try to make one without, you will see an error message such as this.

```sh
ERROR: failed to create cluster: must have at least one control-plane node
```


### Config

Make sure to check the newest version, you can find the images here:
- https://hub.docker.com/r/kindest/node/

I will create a file named `kind-config.yaml` and place the following contents in it:

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
Here we are creating two nodes, one for the control-plane and one worker.


```sh
$ kind create cluster --config=./kind-config.yaml
...
```

You can use a tool such as [kubectx](https://github.com/ahmetb/kubectx) to ensure your context is set.
```sh
$ kubectx
kind-slke-1
```
Notice how the name is prefixed with `kind-`

### See the nodes running


#### Use `docker ps` to see the nodes running
```sh
$ docker ps
CONTAINER ID   IMAGE                  COMMAND                  CREATED         STATUS         PORTS                       NAMES
4b59fab194e5   kindest/node:v1.34.0   "/usr/local/bin/entr…"   2 minutes ago   Up 2 minutes   127.0.0.1:57499->6443/tcp   slke-1-control-plane
bdf56ba1fc15   kindest/node:v1.34.0   "/usr/local/bin/entr…"   2 minutes ago   Up 2 minutes                               slke-1-worker
```

#### `kubectl get nodes`

```sh
$ kubectl get nodes
NAME                   STATUS   ROLES           AGE   VERSION
slke-1-control-plane   Ready    control-plane   15m   v1.34.0
slke-1-worker          Ready    <none>          15m   v1.34.0
```

If it is bothering you that `ROLES` is empty for the `worker` node, you can run:
```sh
$ kubectl label nodes slke-1-worker node-role.kubernetes.io/worker=""
node/slke-1-worker labeled
$ kubectl get nodes
NAME                   STATUS   ROLES           AGE   VERSION
slke-1-control-plane   Ready    control-plane   16m   v1.34.0
slke-1-worker          Ready    worker          16m   v1.34.0
```

### Updating the Cluster

You cannot update a cluster after creation, so you will need to delete it and then recreate it if you are changing any config.


### Setting up `cloud-provider-kind`

`cloud-provider-kind` allows you to use a Kubernetes `Service` type [LoadBalancer](https://kubernetes.io/docs/concepts/services-networking/service/#loadbalancer).

The reason it is called `cloud-provider-kind` is because when you use a Kubernetes `Service` type `LoadBalancer` in a cloud environment, such as [Google Kubernetes Engine](https://docs.cloud.google.com/kubernetes-engine/docs/concepts/service-load-balancer), the Cloud Provider will setup some infrastructure behind the scenes to allow routing.

Previously, you had to configure [MetalLB](https://github.com/metallb/metallb) (which was a more involved setup and had issues on macOS), or use [Extra Port Mappings](https://kind.sigs.k8s.io/docs/user/configuration/#extra-port-mappings) (which was a simple solution, but it felt more hacky and different from what would be used in a production environment).


#### Install

```sh
$ brew install cloud-provider-kind
$ which cloud-provider-kind
/opt/homebrew/bin/cloud-provider-kind
```

#### Run

I recommend opening a separate terminal session for running this command to easily view the logs.

According to the [README of the repository](https://github.com/kubernetes-sigs/cloud-provider-kind/blob/main/README.md), you will need to run with `sudo`
```sh
$ sudo cloud-provider-kind
```

You can watch the log information when a Kubernetes `Service` type `LoadBalancer` is created or deleted.


You can also view the docker container running:

```sh
$ docker ps
CONTAINER ID   IMAGE                      COMMAND                  CREATED          STATUS          PORTS                                                                                                                                     NAMES
a411bd911a94   envoyproxy/envoy:v1.33.2   "/docker-entrypoint.…"   4 seconds ago    Up 3 seconds    0.0.0.0:32774->80/tcp, [::]:32774->80/tcp, 0.0.0.0:32775->443/tcp, [::]:32775->443/tcp, 0.0.0.0:32776->10000/tcp, [::]:32776->10000/tcp   kindccm-f53170ec34f3
...
```


### Deploying `ingress-nginx` as our `LoadBalancer`

#### Creating a values file

I will create a file called `ingress-nginx.values.yaml` with the following contents:

```yaml
controller:
  service:
    type: "LoadBalancer"
```

You can pass values as command line arguments, but I prefer values files as it helps with upgrading the release if needed.

### Deploy the helm chart

Make sure to use the latest chart version, for me it was `4.13.3`.

```sh
$ helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
$ helm repo update
$ helm upgrade --install ingress-nginx ingress-nginx/ingress-nginx \
    --create-namespace \
    -n ingress-nginx \
    --values ingress-nginx.values.yaml \
    --version 4.13.3 \
    --wait
```

#### Confirm installation


##### Check namespace, pod, and service
```sh
$ kubectl get ns
NAME                 STATUS   AGE
default              Active   21m
ingress-nginx        Active   2m12s
kube-node-lease      Active   21m
kube-public          Active   21m
kube-system          Active   21m
local-path-storage   Active   21m
$ kubectl get pods -n ingress-nginx
NAME                                        READY   STATUS    RESTARTS   AGE
ingress-nginx-controller-6f6c964579-csllp   1/1     Running   0          2m27s
$ kubectl get svc -n ingress-nginx
kubectl get svc -n ingress-nginx
NAME                                 TYPE           CLUSTER-IP     EXTERNAL-IP   PORT(S)                      AGE
ingress-nginx-controller             LoadBalancer   10.96.241.10   172.18.0.4    80:32725/TCP,443:30829/TCP   2m51s
ingress-nginx-controller-admission   ClusterIP      10.96.249.13   <none>        443/TCP                      2m51s
```

If your `EXTERNAL-IP` for the `ingress-nginx-controller` svc is showing `<pending>` make sure `cloud-provider-kind` is running and inspect the logs.

---
Easy way to print out your IP address:
```sh
$ kubectl get svc/ingress-nginx-controller -n ingress-nginx -o=jsonpath='{.status.loadBalancer.ingress[0].ip}'
172.18.0.4
```

##### Use `curl` to hit the `LoadBalancer`

```sh
$ curl http://172.18.0.4
<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx</center>
</body>
</html>
```

Awesome!


### Configure `/etc/hosts`

Using the IP address we got from:
```sh
$ kubectl get svc/ingress-nginx-controller -n ingress-nginx -o=jsonpath='{.status.loadBalancer.ingress[0].ip}'
```

We can update `/etc/hosts` to have a nice looking URL.

```sh
$ sudo vim /etc/hosts
...
172.18.0.4      kind.local
```

Now you can go to http://kind.local

### Deploy a Sample App and use the Ingress Controller

I will be using the [hashicorp http-echo](https://github.com/hashicorp/http-echo) server has my example app.

Make sure to use the most up to date image, you can view the images [here](https://hub.docker.com/r/hashicorp/http-echo).

I will create a file called `app.yaml` and add the following manifests in it:

```yaml
---
apiVersion: v1
kind: Namespace
metadata:
  name: app
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: http-echo-deployment
  namespace: app
spec:
  replicas: 1
  selector:
    matchLabels:
      app: http-echo
  template:
    metadata:
      labels:
        app: http-echo
    spec:
      containers:
        - name: http-echo
          image: hashicorp/http-echo:1.0.0
          args: ["-text", "Hello from Kubernetes!", "-listen", ":8080"]
          ports:
            - containerPort: 8080
---
apiVersion: v1
kind: Service
metadata:
  name: http-echo-service
  namespace: app
spec:
  selector:
    app: http-echo
  ports:
    - protocol: TCP
      port: 80
      targetPort: 8080
  type: ClusterIP
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: http-echo-service-ingress
  namespace: app
spec:
  ingressClassName: nginx
  rules:
    - host: kind.local
      http:
        paths:
          - path: /
            pathType: ImplementationSpecific
            backend:
              service:
                name: http-echo-service
                port:
                  number: 80
```

```sh
$ kubectl apply -f app.yaml
namespace/app created
deployment.apps/http-echo-deployment created
service/http-echo-service created
ingress.networking.k8s.io/http-echo-service-ingress created
```

Go to http://kind.local and see the message!


### Cleaning up

stop your `cloud-provider-kind` by Ctrl+C'ing the terminal or by looking the processes and stopping it.

And then delete your cluster:
```sh
$ kind delete cluster --name slke-1
```

## TLDR

Use my provided `Taskfile.yml` and run it all yourself easily!

- https://github.com/standardloop/knowledge-transfer/blob/main/01-kind/Taskfile.yml

```sh
$ task
```
