const config_module = require('./config');
const redis = require('ioredis');

const RedisCli=new redis({
    port: config_module.redis_port,
    host: config_module.redis_host,
    password: config_module.redis_passwd
});

RedisCli.on('error', function(err){
    console.log("RedisCli connect error");
    redisCli.quit();
});

async function GetRedis(key){
    try{
        const result=await RedisCli.get(key);
        if(result==null){
            console.log('result:','<'+result+'>','This key cannot be found')
            return null;
        }
        console.log('Result:','<'+result+'>','Get key successfully');
        return result;
    }catch(err){
        console.log('GetRedis error is',err);
        return null;
    }
}

async function QueryRedis(key){
    try{
        const result=await RedisCli.exists(key);
        if(result==0){
            console.log('result:','<'+result+'>','This key cannot be found')
            return null;
        }
        console.log('Result:','<'+result+'>','This key exists');
        return result;
    }catch(err){
        console.log('QueryRedis error is',err);
        return null;
    }
}

async function SetRedisExpire(key,value, exptime){
    try{
        // 设置键和值
        await RedisCli.set(key,value)
        // 设置过期时间（以秒为单位）
        await RedisCli.expire(key, exptime);
        return true;
    }catch(error){
        console.log('SetRedisExpire error is', error);
        return false;
    }
}

function Quit(){
    RedisCli.quit();
}

module.exports = {GetRedis, QueryRedis, Quit, SetRedisExpire,}