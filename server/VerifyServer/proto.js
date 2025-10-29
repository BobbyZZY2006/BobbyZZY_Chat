const path=require('path');
const grpc=require('@grpc/grpc-js');
const protoLoader=require('@grpc/proto-loader');

const PROTO_PATH=path.join(__dirname,'message.proto');
const packageDefinition=protoLoader.loadSync(PROTO_PATH,{
    keepCase:true,
    longs:String,
    enums:String,
    defaults:true,
    oneofs:true
});
const messageProto=grpc.loadPackageDefinition(packageDefinition).message;

module.exports=messageProto;